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

const UnlockedManifest &
ManifestFileMixin::getUnlockedManifest()
{
  if ( ! this->manifest.has_value() )
    {
      this->manifest = UnlockedManifest( this->getManifestPath() );
    }
  return *this->manifest;
}


/* -------------------------------------------------------------------------- */

const RegistryRaw &
ManifestFileMixin::getLockedRegistry()
{
  if ( ! this->lockedRegistry.has_value() )
    {
      this->lockedRegistry
        = this->getUnlockedManifest().getLockedRegistry( this->getStore() );
    }
  return *this->lockedRegistry;
}


/* -------------------------------------------------------------------------- */

const pkgdb::PkgQueryArgs &
ManifestFileMixin::getBaseQueryArgs()
{
  if ( ! this->baseQueryArgs.has_value() )
    {
      this->baseQueryArgs = this->getUnlockedManifest().getBaseQueryArgs();
    }
  return *this->baseQueryArgs;
}


/* -------------------------------------------------------------------------- */

const std::unordered_map<std::string, std::optional<Resolved>> &
ManifestFileMixin::lockDescriptor( const std::string        &iid,
                                   const ManifestDescriptor &desc )
{
  /* See if it was already locked. */
  if ( auto locked = this->lockedDescriptors.find( iid );
       locked != this->lockedDescriptors.end() )
    {
      return locked->second;
    }

  /* For each system we need to set `packageRepository' such that it has a `rev'
   * field, and set `absPath` to the absolute attribute path the
   * resolved package. */
  std::unordered_map<std::string, std::optional<Resolved>> locked;

  pkgdb::PkgQueryArgs baseArgs = this->getBaseQueryArgs();

  /* Iterate over the systems. */
  for ( const auto &system : this->getSystems() )
    {
      /* See if this system is skippable. */
      if ( desc.systems.has_value()
           && std::find( desc.systems->begin(), desc.systems->end(), system )
                == desc.systems->end() )
        {
          /* This descriptor is not for this system. */
          locked.emplace( system, std::nullopt );
          continue;
        }

      /* Otherwise, try to lock it with an input. */
      if ( desc.input.has_value() )
        {
          // TODO: Post-GA this is allowed!
          throw InvalidManifestFileException(
            "You cannot specify an input in manifest files." );
        }

      /* Prep our query. */
      pkgdb::PkgQueryArgs args = baseArgs;
      desc.fillPkgQueryArgs( args );
      args.systems = { system };
      pkgdb::PkgQuery query( args );

      // TODO: handle groups.

      /* Try each input. */
      bool found = false;
      for ( const auto &[name, input] : *this->getPkgDbRegistry() )
        {
          auto rows = query.execute( input->getDbReadOnly()->db );
          if ( rows.empty() ) { continue; }

          found         = true;
          auto     info = input->getDbReadOnly()->getPackage( rows.front() );
          Resolved resolved {
            .input = Resolved::Input( name, *input->getFlakeRef() ),
            .path  = std::move( info.at( "absPath" ) ),
            .info  = { { "pname", std::move( info.at( "pname" ) ) },
                       { "version", std::move( info.at( "version" ) ) },
                       { "license", std::move( info.at( "license" ) ) },
                       { "priority", desc.priority } }
          };

          locked.emplace( system, std::move( resolved ) );
          break;
        }

      if ( ! found )
        {
          /* No match. */
          if ( desc.optional )
            {
              locked.emplace( system, std::nullopt );
              continue;
            }
          // TODO: make this a typed exception.
          throw FloxException( "Failed to resolve descriptor for `install."
                               + iid + "' on system `" + system + "'." );
        }
    }

  /* Stash it in the collection. */
  this->lockedDescriptors.emplace( iid, std::move( locked ) );

  return this->lockedDescriptors.at( iid );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
