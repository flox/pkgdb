/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/resolver/manifest-raw.hh"
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
GlobalManifest::initRegistry()
{
  this->manifestRaw.check();
  if ( this->manifestRaw.registry.has_value() )
    {
      this->registryRaw = *this->manifestRaw.registry;
    }
}


/* -------------------------------------------------------------------------- */

GlobalManifest::GlobalManifest( std::filesystem::path manifestPath )
  : manifestRaw( readManifestFromPath<GlobalManifestRaw>( manifestPath ) )
{
  this->initRegistry();
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
  std::optional<std::vector<std::string>> maybeSystems;
  if ( auto maybeOpts = this->getManifestRaw().options; maybeOpts.has_value() )
    {
      maybeSystems = maybeOpts->systems;
    }

  for ( const auto & [iid, desc] : this->descriptors )
    {
      if ( ! desc.systems.has_value() ) { continue; }
      if ( ! maybeSystems.has_value() )
        {
          throw InvalidManifestFileException(
            "descriptor `install." + iid
            + "' specifies `systems' but no `options.systems' are specified"
              " in the manifest." );
        }
      for ( const auto & system : *desc.systems )
        {
          if ( std::find( maybeSystems->begin(), maybeSystems->end(), system )
               == maybeSystems->end() )
            {
              throw InvalidManifestFileException(
                "descriptor `install." + iid + "' specifies system `"
                + system
                + "' which is not in `options.systems' in the manifest." );
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

void
Manifest::initDescriptors()
{
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

Manifest::Manifest( std::filesystem::path manifestPath )
  : GlobalManifest( readManifestFromPath<ManifestRaw>( manifestPath ) )
{
  this->initDescriptors();
}


/* -------------------------------------------------------------------------- */

std::vector<InstallDescriptors>
Manifest::getGroupedDescriptors() const
{
  /* Group all packages into a map with group name as key. */
  std::unordered_map<GroupName, InstallDescriptors> grouped;
  InstallDescriptors                                defaultGroup;
  for ( const auto & [iid, desc] : this->descriptors )
    {
      // TODO: Use manifest options to decide how ungrouped descriptors
      //       are grouped.
      /* For now add all descriptors without a group to `defaultGroup`. */
      if ( ! desc.group.has_value() ) { defaultGroup.emplace( iid, desc ); }
      else
        {
          grouped.try_emplace( *desc.group, InstallDescriptors {} );
          grouped.at( *desc.group ).emplace( iid, desc );
        }
    }

  /* Add all groups to a vector.
   * Don't use a map with group name because the defaultGroup doesn't have
   * a name. */
  std::vector<InstallDescriptors> allDescriptors;
  if ( ! defaultGroup.empty() ) { allDescriptors.emplace_back( defaultGroup ); }
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
