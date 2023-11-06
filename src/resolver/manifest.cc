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

#include "compat/concepts.hh"
#include "flox/core/util.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Read a flox::resolver::GlobalManifestRaw or
 *        flox::resolver::ManifestRaw from a file.
 */
template<typename ManifestType>
static ManifestType
readManifestFromPath( const std::filesystem::path &manifestPath )
  requires std::is_base_of<GlobalManifestRaw, ManifestType>::value
{
  if ( ! std::filesystem::exists( manifestPath ) )
    {
      throw InvalidManifestFileException( "no such path: "
                                          + manifestPath.string() );
    }
  return readAndCoerceJSON( manifestPath );
}


/* -------------------------------------------------------------------------- */

void
GlobalManifest::init()
{
  this->manifestRaw.check();
  if ( this->manifestRaw.registry.has_value() )
    {
      this->registryRaw = *this->manifestRaw.registry;
    }
}


/* -------------------------------------------------------------------------- */

GlobalManifest::GlobalManifest( std::filesystem::path manifestPath )
  : manifestPath( std::move( manifestPath ) )
  , manifestRaw( readManifestFromPath<GlobalManifestRaw>( this->manifestPath ) )
{
  this->init();
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs
GlobalManifest::getBaseQueryArgs() const
{
  pkgdb::PkgQueryArgs args;
  if ( ! this->manifestRaw.options.has_value() ) { return args; }

  if ( this->manifestRaw.options->systems.has_value() )
    {
      args.systems = *this->manifestRaw.options->systems;
    }

  if ( this->manifestRaw.options->allow.has_value() )
    {
      if ( this->manifestRaw.options->allow->unfree.has_value() )
        {
          args.allowUnfree = *this->manifestRaw.options->allow->unfree;
        }
      if ( this->manifestRaw.options->allow->broken.has_value() )
        {
          args.allowBroken = *this->manifestRaw.options->allow->broken;
        }
      args.licenses = this->manifestRaw.options->allow->licenses;
    }

  if ( this->manifestRaw.options->semver.has_value()
       && this->manifestRaw.options->semver->preferPreReleases.has_value() )
    {
      args.preferPreReleases
        = *this->manifestRaw.options->semver->preferPreReleases;
    }
  return args;
}


/* -------------------------------------------------------------------------- */

void
Manifest::check() const
{
  this->manifestRaw.check();
  for ( const auto &[iid, desc] : this->descriptors )
    {
      if ( desc.systems.has_value() )
        {
          if ( ( ! this->manifestRaw.options.has_value() )
               || ( ! this->manifestRaw.options->systems.has_value() ) )
            {
              throw InvalidManifestFileException(
                "descriptor `install." + iid
                + "' specifies `systems' but no `options.systems' are specified"
                  " in the manifest." );
            }
          for ( const auto &system : *desc.systems )
            {
              if ( std::find( this->manifestRaw.options->systems->begin(),
                              this->manifestRaw.options->systems->end(),
                              system )
                   == this->manifestRaw.options->systems->end() )
                {
                  throw InvalidManifestFileException(
                    "descriptor `install." + iid + "' specifies system `"
                    + system
                    + "' which is not in `options.systems' in the manifest." );
                }
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

void
Manifest::init()
{
  this->manifestRaw.check();
  if ( this->manifestRaw.registry.has_value() )
    {
      this->registryRaw = *this->manifestRaw.registry;
    }

  if ( ! this->manifestRaw.install.has_value() ) { return; }
  for ( const auto &[iid, raw] : *this->manifestRaw.install )
    {
      /* An empty/null descriptor uses `name' of the attribute. */
      if ( raw.has_value() )
        {
          this->descriptors.emplace( iid, ManifestDescriptor( iid, *raw ) );
        }
      else
        {
          ManifestDescriptor manDesc;
          manDesc.name = iid;
          this->descriptors.emplace( iid, std::move( manDesc ) );
        }
    }
  this->check();
}


/* -------------------------------------------------------------------------- */

Manifest::Manifest( std::filesystem::path manifestPath, ManifestRaw raw )
{
  this->manifestPath = std::move( manifestPath );
  this->manifestRaw  = std::move( raw );
  this->init();
}


/* -------------------------------------------------------------------------- */

Manifest::Manifest( std::filesystem::path manifestPath )
{
  this->manifestPath = std::move( manifestPath );
  this->manifestRaw  = readManifestFromPath<ManifestRaw>( this->manifestPath );
  this->init();
}


/* -------------------------------------------------------------------------- */

std::unordered_map<GroupName, InstallDescriptors>
Manifest::getGroupedDescriptors() const
{
  std::unordered_map<GroupName, InstallDescriptors> grouped;
  for ( const auto &[iid, desc] : this->descriptors )
    {
      if ( ! desc.group.has_value() ) { continue; }
      grouped.try_emplace( *desc.group, InstallDescriptors {} );
      grouped.at( *desc.group ).emplace( iid, desc );
    }
  return grouped;
}


/* -------------------------------------------------------------------------- */

InstallDescriptors
Manifest::getUngroupedDescriptors() const
{
  InstallDescriptors ungrouped;
  for ( const auto &[iid, desc] : this->descriptors )
    {
      if ( ! desc.group.has_value() ) { ungrouped.emplace( iid, desc ); }
    }
  return ungrouped;
}


/* -------------------------------------------------------------------------- */

// TODO: Migrate to `Lockfile'
#if 0
void
ManifestFileMixin::checkGroups()
{
  std::unordered_map<GroupName,
                     std::unordered_map<System, std::optional<Resolved::Input>>>
    groupInputs;
  for ( const auto &[iid, systemsResolved] : this->lockedDescriptors )
    {
      auto maybeGroupName = this->getDescriptors().at( iid ).group;
      /* Skip if the descriptor doesn't name a group. */
      if ( ! maybeGroupName.has_value() ) { continue; }
      /* Either define the group or verify alignment. */
      auto maybeGroup = groupInputs.find( *maybeGroupName );
      if ( maybeGroup == groupInputs.end() )
        {
          std::unordered_map<std::string, std::optional<Resolved::Input>>
            inputs;
          for ( const auto &[system, resolved] : systemsResolved )
            {
              if ( resolved.has_value() )
                {
                  inputs.emplace( system, resolved->input );
                }
              else { inputs.emplace( system, std::nullopt ); }
            }
          groupInputs.emplace( maybeGroup->first, std::move( inputs ) );
        }
      else
        {
          for ( const auto &[system, resolved] : systemsResolved )
            {
              if ( resolved.has_value() )
                {
                  /* If the previous declaration skipped a system, we
                   * may fill it.
                   * Otherwise assert equality. */
                  if ( ! maybeGroup->second.at( system ).has_value() )
                    {
                      maybeGroup->second.at( system ) = resolved->input;
                    }
                  else if ( ( *maybeGroup->second.at( system ) ).locked
                            != resolved->input.locked )
                    {
                      // TODO: make a new exception
                      throw FloxException(
                        "locked descriptor `packages." + iid + "." + system
                        + "' does not align with other members of its group" );
                    }
                }
            }
        }
    }
}
#endif


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
