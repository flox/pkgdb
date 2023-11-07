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
readManifestFromPath( const std::filesystem::path & manifestPath )
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
  for ( const auto & [iid, desc] : this->descriptors )
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
          for ( const auto & system : *desc.systems )
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
  for ( const auto & [iid, raw] : *this->manifestRaw.install )
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

std::vector<InstallDescriptors>
Manifest::getGroupedDescriptors() const
{
  // Group all packages into a map with group name as key.
  std::unordered_map<GroupName, InstallDescriptors> grouped;
  InstallDescriptors                                defaultGroup;
  for ( const auto & [iid, desc] : this->descriptors )
    {
      // For now add all descriptors without a group to `defaultGroup`.
      // TODO: use manifest options to decide how ungrouped descriptors are
      // grouped.
      if ( ! desc.group.has_value() ) { defaultGroup.emplace( iid, desc ); }
      else
        {
          grouped.try_emplace( *desc.group, InstallDescriptors {} );
          grouped.at( *desc.group ).emplace( iid, desc );
        }
    }

  // Add all groups to a vector. Don't use a map with group name because the
  // defaultGroup doesn't have a name.
  std::vector<InstallDescriptors> allDescriptors;
  allDescriptors.emplace_back( defaultGroup );
  for ( const auto & [_, group] : grouped )
    {
      allDescriptors.emplace_back( group );
    }
  return allDescriptors;
}

/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
