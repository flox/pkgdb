/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <optional>

#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

static ManifestRaw
readManifestFromPath( const std::filesystem::path &manifestPath )
{
  if ( ! std::filesystem::exists( manifestPath ) )
    {
      throw InvalidManifestFileException( "no such path: "
                                          + manifestPath.string() );
    }
  return readAndCoerceJSON( manifestPath );
}


/* -------------------------------------------------------------------------- */

UnlockedManifest::UnlockedManifest( std::filesystem::path manifestPath )
  : manifestPath( std::move( manifestPath ) )
  , manifestRaw( readManifestFromPath( this->manifestPath ) )
{}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs
UnlockedManifest::getBaseQueryArgs() const
{
  pkgdb::PkgQueryArgs args;
  if ( this->manifestRaw.options.systems.has_value() )
    {
      args.systems = *this->manifestRaw.options.systems;
    }

  if ( this->manifestRaw.options.allow.has_value() )
    {
      if ( this->manifestRaw.options.allow->unfree.has_value() )
        {
          args.allowUnfree = *this->manifestRaw.options.allow->unfree;
        }
      if ( this->manifestRaw.options.allow->broken.has_value() )
        {
          args.allowBroken = *this->manifestRaw.options.allow->broken;
        }
      args.licenses = this->manifestRaw.options.allow->licenses;
    }

  if ( this->manifestRaw.options.semver.has_value()
       && this->manifestRaw.options.semver->preferPreReleases.has_value() )
    {
      args.preferPreReleases
        = *this->manifestRaw.options.semver->preferPreReleases;
    }
  return args;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
ManifestFileMixin::addManifestFileOption( argparse::ArgumentParser &parser )
{
  return parser.add_argument( "--manifest" )
    .help( "The path to the 'manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string &strPath )
             { this->manifestPath = nix::absPath( strPath ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
ManifestFileMixin::addManifestFileArg( argparse::ArgumentParser &parser,
                                       bool                      required )
{
  argparse::Argument &arg
    = parser.add_argument( "manifest" )
        .help( "The path to a 'manifest.{toml,yaml,json}' file." )
        .metavar( "MANIFEST-PATH" )
        .action( [&]( const std::string &strPath )
                 { this->manifestPath = nix::absPath( strPath ); } );
  return required ? arg.required() : arg;
}


/* -------------------------------------------------------------------------- */

std::filesystem::path
ManifestFileMixin::getManifestPath()
{
  if ( ! this->manifestPath.has_value() )
    {
      throw InvalidManifestFileException( "no manifest path given" );
    }
  return *this->manifestPath;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
