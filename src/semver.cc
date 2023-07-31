/* ========================================================================== *
 *
 *
 * -------------------------------------------------------------------------- */

#include "semver.hh"
#include <string>
#include <regex>
#include <optional>


/* -------------------------------------------------------------------------- */

/** Interfaces for analyzing version numbers */
namespace versions {

/* -------------------------------------------------------------------------- */

/* Matches Semantic Version strings, e.g. `4.2.0-pre' */
#define _re_vp "(0|[1-9][0-9]*)"
static const std::regex semverRE(
  _re_vp "\\." _re_vp "\\." _re_vp "(-[-[:alnum:]_+.]+)?"
, std::regex::ECMAScript
);

/* Coercively matches Semantic Version Strings, e.g. `v1.0-pre' */
static const std::regex semverCoerceRE(
  "(.*@)?[vV]?(0*([0-9]+)(\\.0*([0-9]+)(\\.0*([0-9]+))?)?(-[-[:alnum:]_+.]+)?)"
, std::regex::ECMAScript
);

/* Match '-' separated date strings, e.g. `2023-05-31' or `5-1-23' */
static const std::regex dateRE(
  "([0-9][0-9]([0-9][0-9])?-[01]?[0-9]-[0-9][0-9]?|"
  "[0-9][0-9]?-[0-9][0-9]?-[0-9][0-9]([0-9][0-9])?)(-[-[:alnum:]_+.]+)?"
, std::regex::ECMAScript
);


/* -------------------------------------------------------------------------- */

  bool
isSemver( const std::string & version )
{
  return std::regex_match( version, semverRE );
}

  bool
isSemver( std::string_view version )
{
  std::string v( version );
  return std::regex_match( v, semverRE );
}


/* -------------------------------------------------------------------------- */

  bool
isDate( const std::string & version )
{
  return std::regex_match( version, dateRE );
}

  bool
isDate( std::string_view version )
{
  std::string v( version );
  return std::regex_match( v, dateRE );
}


/* -------------------------------------------------------------------------- */

  bool
isCoercibleToSemver( const std::string & version )
{
  return ( ! std::regex_match( version, dateRE ) ) &&
         std::regex_match( version, semverCoerceRE );
}

  bool
isCoercibleToSemver( std::string_view version )
{
  std::string v( version );
  return ( ! std::regex_match( v, dateRE ) ) &&
         std::regex_match( v, semverCoerceRE );
}


/* -------------------------------------------------------------------------- */

  std::optional<std::string>
coerceSemver( std::string_view version )
{
  std::string v( version );
  /* If it's already a match for a proper semver we're done. */
  if ( std::regex_match( v, semverRE ) )
    {
      return std::optional<std::string>( v );
    }

  /* Try try matching the coercive pattern. */
  std::smatch sm;
  if ( isDate( v ) || ( ! std::regex_match( v, sm, semverCoerceRE ) ) )
    {
      return std::nullopt;
    }

  #if defined( DEBUG ) && ( DEBUG != 0 )
  for ( unsigned int i = 0; i < sm.size(); ++i )
    {
      std::cerr << "[" << i << "]: " << sm[i] << std::endl;
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

  /* The `str()' function is destructive and works by adding null terminators to
   * the original string.
   * If we attempt to convert each submatch from left to right we will clobber
   * some characters with null terminators.
   * To avoid this we convert each submatch to a string from right to left.
   */
  std::string tag( sm[8].str() );
  std::string patch( sm[7].str() );
  std::string minor( sm[5].str() );

  std::string rsl( sm[3].str() + "." );

  if ( minor.empty() ) { rsl += "0."; }
  else                 { rsl += minor + "."; }

  if ( patch.empty() ) { rsl += "0"; }
  else                 { rsl += patch; }

  if ( ! tag.empty() ) { rsl += tag; }

  return std::optional<std::string>( rsl );
}


/* -------------------------------------------------------------------------- */

#ifndef SEMVER_PATH
#  define SEMVER_PATH  semver
#endif
#define _XSTRIZE( _S )   _STRIZE( _S )
#define _STRIZE( _S )    # _S
#define SEMVER_PATH_STR  _XSTRIZE( SEMVER_PATH )

  std::pair<int, std::string>
runSemver( const std::list<std::string> & args )
{
  static const std::string semverProg =
    nix::getEnv( "SEMVER" ).value_or( SEMVER_PATH_STR );
  static const std::map<std::string, std::string> env = nix::getEnv();
  return nix::runProgram( {
    .program     = semverProg
  , .searchPath  = true
  , .args        = args
  , .environment = env
  } );
}


/* -------------------------------------------------------------------------- */

  std::list<std::string>
semverSat( const std::string & range, const std::list<std::string> & versions )
{
  std::list<std::string> args = {
    "--include-prerelease", "--loose", "--range", range
  };
  for ( auto & v : versions ) { args.push_back( v ); }
  auto [ec, lines] = runSemver( args );
  if ( ! nix::statusOk( ec ) ) { return {}; }
  std::list<std::string> rsl;
  std::stringstream ss( lines );
  std::string l;
  while ( std::getline( ss, l, '\n' ) )
    {
      if ( ! l.empty() ) { rsl.push_back( std::move( l ) ); }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

}  /* End namespace `versions' */

/* -------------------------------------------------------------------------- *
 *
 *
 * ========================================================================== */
