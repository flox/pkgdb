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

/* Instantiate templates. */

template class ManifestBase<ManifestRaw>;

template class ManifestBase<GlobalManifestRaw>;


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
                "descriptor `install." + iid + "' specifies system `" + system
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
