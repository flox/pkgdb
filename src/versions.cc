/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <regex>
#include <optional>
#include <stdexcept>

#include "versions.hh"
#include "date.hh"


/* -------------------------------------------------------------------------- */

/** Interfaces for analyzing version numbers */
namespace versions {

/* -------------------------------------------------------------------------- */

/* Matches Semantic Version strings, e.g. `4.2.0-pre' */
static const char * const semverREStr =
  "(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(-[-[:alnum:]_+.]+)?";

/* Coercively matches Semantic Version Strings, e.g. `v1.0-pre' */
static const char * const semverCoerceREStr =
  "(.*@)?[vV]?(0*([0-9]+)(\\.0*([0-9]+)(\\.0*([0-9]+))?)?(-[-[:alnum:]_+.]+)?)";

/* Match '-' separated date strings, e.g. `2023-05-31' or `5-1-23' */
static const char * const dateREStr =
  "([0-9][0-9]([0-9][0-9])?-[01]?[0-9]-[0-9][0-9]?|"
  "[0-9][0-9]?-[0-9][0-9]?-[0-9][0-9]([0-9][0-9])?)(-[-[:alnum:]_+.]+)?";


/* -------------------------------------------------------------------------- */

  bool
isSemver( const std::string & version )
{
  static const std::regex semverRE( semverREStr, std::regex::ECMAScript );
  return std::regex_match( version, semverRE );
}

  bool
isSemver( std::string_view version )
{
  std::string vsn( version );
  static const std::regex semverRE( semverREStr, std::regex::ECMAScript );
  return std::regex_match( vsn, semverRE );
}


/* -------------------------------------------------------------------------- */

  bool
isDate( const std::string & version )
{
  static const std::regex dateRE( dateREStr, std::regex::ECMAScript );
  return std::regex_match( version, dateRE );
}

  bool
isDate( std::string_view version )
{
  static const std::regex dateRE( dateREStr, std::regex::ECMAScript );
  std::string vsn( version );
  return std::regex_match( vsn, dateRE );
}


/* -------------------------------------------------------------------------- */

  bool
compareSemVersLT( const std::string & lhs
                , const std::string & rhs
                ,       bool          preferPreReleases
                )
{
  static const std::regex semverRE( semverREStr, std::regex::ECMAScript );
  static const size_t     majorIdx  = 1;
  static const size_t     minorIdx  = 2;
  static const size_t     patchIdx  = 3;
  static const size_t     preTagIdx = 4;

  std::smatch matchL;
  if ( ! std::regex_match( lhs, matchL, semverRE ) )
    {
      throw VersionException(
        "'" + lhs + "' is not a semantic version string"
      );
    }

  std::smatch matchR;
  if ( ! std::regex_match( rhs, matchR, semverRE ) )
    {
      throw VersionException(
        "'" + rhs + "' is not a semantic version string"
      );
    }

  if ( ! preferPreReleases )
    {
      bool isPreL = ! matchL[preTagIdx].str().empty();
      bool isPreR = ! matchL[preTagIdx].str().empty();
      if ( isPreL != isPreR ) { return isPreL; }
    }

  /* Major */
  unsigned valL = std::stoul( matchL[majorIdx].str() );
  unsigned valR = std::stoul( matchR[majorIdx].str() );
  if ( valL < valR ) { return true; } else if ( valR < valL ) { return false; }

  /* Minor */
  valL = std::stoul( matchL[minorIdx].str() );
  valR = std::stoul( matchR[minorIdx].str() );
  if ( valL < valR ) { return true; } else if ( valR < valL ) { return false; }

  /* Patch */
  valL = std::stoul( matchL[patchIdx].str() );
  valR = std::stoul( matchR[patchIdx].str() );
  if ( valL < valR ) { return true; } else if ( valR < valL ) { return false; }

  return matchL[preTagIdx].str() < matchR[preTagIdx].str();
}


/* -------------------------------------------------------------------------- */

  bool
compareDateVersLT( const std::string & lhs, const std::string & rhs )
{
  datetime::Date lhd( lhs );
  if ( lhs == lhd.rest )
    {
      throw VersionException( "'" + lhs + "' is not a date string" );
    }

  datetime::Date rhd( rhs );
  if ( rhs == rhd.rest )
    {
      throw VersionException( "'" + rhs + "' is not a date string" );
    }

  /* Uses any trailing characters to break ties lexicographically. */
  return lhd.isBefore( rhd, false );
}


/* -------------------------------------------------------------------------- */

  bool
compareVersionsLT( const std::string & lhs
                 , const std::string & rhs
                 ,       bool          preferPreReleases
                 )
{
  const version_kind vkl = getVersionKind( lhs );
  const version_kind vkr = getVersionKind( rhs );

  if ( vkl != vkr ) { return vkl < vkr; }

  if ( vkl == VK_SEMVER )
    {
      return compareSemVersLT( lhs, rhs, preferPreReleases );
    }
  if ( vkl == VK_DATE ) { return compareDateVersLT( lhs, rhs ); }
  assert( vkl == VK_OTHER );
  return lhs <= rhs;
}


/* -------------------------------------------------------------------------- */

  bool
isCoercibleToSemver( const std::string & version )
{
  static const std::regex dateRE( dateREStr, std::regex::ECMAScript );
  static const std::regex semverCoerceRE( semverCoerceREStr
                                        , std::regex::ECMAScript
                                        );
  return ( ! std::regex_match( version, dateRE ) ) &&
         std::regex_match( version, semverCoerceRE );
}

  bool
isCoercibleToSemver( std::string_view version )
{
  static const std::regex dateRE( dateREStr, std::regex::ECMAScript );
  static const std::regex semverCoerceRE( semverCoerceREStr
                                        , std::regex::ECMAScript
                                        );
  std::string vsn( version );
  return ( ! std::regex_match( vsn, dateRE ) ) &&
         std::regex_match( vsn, semverCoerceRE );
}


/* -------------------------------------------------------------------------- */

  std::optional<std::string>
coerceSemver( std::string_view version )
{
  static const std::regex semverRE( semverREStr, std::regex::ECMAScript );
  static const std::regex semverCoerceRE( semverCoerceREStr
                                        , std::regex::ECMAScript
                                        );
  std::string vsn( version );
  /* If it's already a match for a proper semver we're done. */
  if ( std::regex_match( vsn, semverRE ) ) { return { vsn }; }

  /* Try try matching the coercive pattern. */
  std::smatch match;
  if ( isDate( vsn ) || ( ! std::regex_match( vsn, match, semverCoerceRE ) ) )
    {
      return std::nullopt;
    }

  #if defined( DEBUG ) && ( DEBUG != 0 )
  for ( unsigned int i = 0; i < match.size(); ++i )
    {
      std::cerr << "[" << i << "]: " << match[i] << std::endl;
    }
  #endif

  /**
   * Capture Groups Example:
   *   [0]: foo@v1.02.0-pre
   *   [1]: foo@
   *   [2]: 1.02.0-pre
   *   [3]: 1
   *   [4]: .02.0
   *   [5]: 2
   *   [6]: .0
   *   [7]: 0
   *   [8]: -pre
   */
  static const size_t majorIdx  = 3;
  static const size_t minorIdx  = 5;
  static const size_t patchIdx  = 7;
  static const size_t preTagIdx = 8;


  /* The `str()' function is destructive and works by adding null terminators to
   * the original string.
   * If we attempt to convert each submatch from left to right we will clobber
   * some characters with null terminators.
   * To avoid this we convert each submatch to a string from right to left.
   */
  std::string tag( match[preTagIdx].str() );
  std::string patch( match[patchIdx].str() );
  std::string minor( match[minorIdx].str() );

  std::string rsl( match[majorIdx].str() + "." );

  if ( minor.empty() ) { rsl += "0."; }
  else                 { rsl += minor + "."; }

  if ( patch.empty() ) { rsl += "0"; }
  else                 { rsl += patch; }

  if ( ! tag.empty() ) { rsl += tag; }

  return { rsl };
}


/* -------------------------------------------------------------------------- */

#ifndef SEMVER_PATH
#  define SEMVER_PATH  "semver"
#endif

  std::pair<int, std::string>
runSemver( const std::list<std::string> & args )
{
  static const std::string semverProg =
    nix::getEnv( "SEMVER" ).value_or( SEMVER_PATH );
  static const std::map<std::string, std::string> env = nix::getEnv();
  return nix::runProgram( nix::RunOptions {
    .program             = semverProg
  , .searchPath          = true
  , .args                = args
  , .uid                 = std::nullopt
  , .gid                 = std::nullopt
  , .chdir               = std::nullopt
  , .environment         = env
  , .input               = std::nullopt
  , .standardIn          = nullptr
  , .standardOut         = nullptr
  , .mergeStderrToStdout = false
  } );
}


/* -------------------------------------------------------------------------- */

  std::list<std::string>
semverSat( const std::string & range, const std::list<std::string> & versions )
{
  std::list<std::string> args = {
    "--include-prerelease", "--loose", "--range", range
  };
  for ( const auto & version : versions ) { args.push_back( version ); }
  auto [ec, lines] = runSemver( args );
  if ( ! nix::statusOk( ec ) ) { return {}; }
  std::list<std::string> rsl;
  std::stringstream      oss( lines );
  std::string            line;
  while ( std::getline( oss, line, '\n' ) )
    {
      if ( ! line.empty() ) { rsl.push_back( std::move( line ) ); }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

}  /* End namespace `versions' */


/* -------------------------------------------------------------------------- *
 *
 *
 * ========================================================================== */
