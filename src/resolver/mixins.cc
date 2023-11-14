/* ========================================================================== *
 *
 * @file resolver/mixins.cc
 *
 * @brief State blobs for flox commands.
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <argparse/argparse.hpp>
#include <nix/util.hh>

#include "flox/resolver/environment.hh"
#include "flox/resolver/lockfile.hh"
#include "flox/resolver/manifest-raw.hh"
#include "flox/resolver/manifest.hh"
#include "flox/resolver/mixins.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

const std::optional<std::filesystem::path> &
EnvironmentMixin::getGlobalManifestPath() const
{
  return this->globalManifestPath;
}


/* -------------------------------------------------------------------------- */

const std::optional<GlobalManifest> &
EnvironmentMixin::getGlobalManifest()
{
  if ( ( ! this->globalManifest.has_value() )
       && this->globalManifestPath.has_value() )
    {
      this->globalManifest = GlobalManifest( *this->globalManifestPath );
    }
  return this->globalManifest;
}


/* -------------------------------------------------------------------------- */

const std::optional<std::filesystem::path> &
EnvironmentMixin::getManifestPath() const
{
  return this->manifestPath;
}


/* -------------------------------------------------------------------------- */

const Manifest &
EnvironmentMixin::getManifest()
{
  if ( ! this->manifest.has_value() )
    {
      if ( ! this->manifestPath.has_value() )
        {
          throw EnvironmentMixinException(
            "you must provide an inline manifest or the path to a manifest "
            "file" );
        }
      this->manifest = Manifest( *this->manifestPath );
    }
  return *this->manifest;
}


/* -------------------------------------------------------------------------- */

const std::optional<std::filesystem::path> &
EnvironmentMixin::getLockfilePath() const
{
  return this->manifestPath;
}


/* -------------------------------------------------------------------------- */

const std::optional<Lockfile> &
EnvironmentMixin::getLockfile()
{
  if ( ( ! this->lockfile.has_value() ) && this->lockfilePath.has_value() )
    {
      this->lockfile = Lockfile( *this->lockfilePath );
    }
  return this->lockfile;
}


/* -------------------------------------------------------------------------- */

Environment &
EnvironmentMixin::getEnvironment()
{
  if ( ! this->environment.has_value() )
    {
      this->environment = Environment( this->getGlobalManifest(),
                                       this->getManifest(),
                                       this->getLockfile() );
    }
  return *this->environment;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addGlobalManifestFileOption(
  argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--global-manifest" )
    .help( "The path to the user's global `manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initGlobalManifestPath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addManifestFileOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--manifest" )
    .help( "The path to the `manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initManifestPath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addManifestFileArg( argparse::ArgumentParser & parser,
                                      bool                       required )
{
  argparse::Argument & arg
    = parser.add_argument( "manifest" )
        .help( "The path to the project's `manifest.{toml,yaml,json}' file." )
        .metavar( "MANIFEST-PATH" )
        .action( [&]( const std::string & strPath )
                 { this->initManifestPath( nix::absPath( strPath ) ); } );
  return required ? arg.required() : arg;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addLockfileOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--lockfile" )
    .help( "The path to the projects existing `manifest.lock' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initLockfilePath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Generate exception handling boilerplate for
 *        `EnvironmentMixin::init<MEMBER>' functions.
 */
#define ENV_MIXIN_THROW_IF_SET( member )                               \
  if ( this->member.has_value() )                                      \
    {                                                                  \
      throw EnvironmentMixinException( "`" #member                     \
                                       "' was already initializaed" ); \
    }                                                                  \
  if ( this->environment.has_value() )                                 \
    {                                                                  \
      throw EnvironmentMixinException(                                 \
        "`" #member "' cannot be initializaed after `environment'" );  \
    }


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initGlobalManifestPath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( globalManifestPath )
  this->globalManifestPath = std::move( path );
}

void
EnvironmentMixin::initGlobalManifest( GlobalManifestRaw manifestRaw )
{
  ENV_MIXIN_THROW_IF_SET( globalManifest )
  this->globalManifest = GlobalManifest( std::move( manifestRaw ) );
}

void
EnvironmentMixin::initGlobalManifest( GlobalManifest manifest )
{
  ENV_MIXIN_THROW_IF_SET( globalManifest )
  this->globalManifest = std::move( manifest );
}


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initManifestPath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( manifestPath )
  this->manifestPath = std::move( path );
}

void
EnvironmentMixin::initManifest( ManifestRaw manifestRaw )
{
  ENV_MIXIN_THROW_IF_SET( manifest )
  this->manifest = Manifest( std::move( manifestRaw ) );
}

void
EnvironmentMixin::initManifest( Manifest manifest )
{
  ENV_MIXIN_THROW_IF_SET( manifest )
  this->manifest = std::move( manifest );
}


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initLockfilePath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( lockfilePath )
  this->lockfilePath = std::move( path );
}

void
EnvironmentMixin::initLockfile( LockfileRaw lockfileRaw )
{
  ENV_MIXIN_THROW_IF_SET( lockfile );
  this->lockfile = Lockfile( std::move( lockfileRaw ) );
}

void
EnvironmentMixin::initLockfile( Lockfile lockfile )
{
  ENV_MIXIN_THROW_IF_SET( lockfile )
  this->lockfile = std::move( lockfile );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
