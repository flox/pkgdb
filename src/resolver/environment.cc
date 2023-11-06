/* ========================================================================== *
 *
 * @file resolver/environment.cc
 *
 * @brief A collection of files associated with an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/environment.hh"
#include "flox/core/util.hh"
#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

RegistryRaw &
Environment::getCombinedRegistryRaw()
{
  if ( ! this->combinedRegistryRaw.has_value() )
    {
      this->combinedRegistryRaw = this->globalManifest->getRegistryRaw();
      // TODO: merge registries
      // this->combinedRegistryRaw->merge( this->manifest.registryRaw );
    }
  return this->combinedRegistryRaw.value();
}


/* -------------------------------------------------------------------------- */

nix::ref<Registry<pkgdb::PkgDbInputFactory>>
Environment::getPkgDbRegistry()
{
  if ( this->dbs == nullptr )
    {
      if ( this->dbFactory == nullptr )
        {
          nix::ref<nix::Store> store = this->getStore();
          this->dbFactory = std::make_shared<pkgdb::PkgDbInputFactory>( store );
        }
      this->dbs = std::make_shared<Registry<pkgdb::PkgDbInputFactory>>(
        this->getCombinedRegistryRaw(),
        *this->dbFactory );
    }
  return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>( this->dbs );
}


/* -------------------------------------------------------------------------- */

void
Environment::fillLockedFromOldLockfile()
{
  if ( ! this->oldLockfile.has_value() ) { return; }

  for ( const auto & system : this->manifest.getSystems() )
    {
      SystemPackages sysPkgs;
      for ( const auto & [iid, desc] : this->getManifest().getDescriptors() )
        {
          /* Package is skipped for this system, so fill `std::nullopt'. */
          if ( desc.systems.has_value()
               && ( std::find( desc.systems->begin(),
                               desc.systems->end(),
                               system )
                    == desc.systems->end() ) )
            {
              sysPkgs.emplace( iid, std::nullopt );
              continue;
            }

          /* Check to see if the descriptor was modified in such a way that we
           * need to re-resolve. */
          auto oldDesc = this->oldLockfile->getDescriptors().find( iid );

          /* Skip new descriptors. */
          if ( oldDesc == this->oldLockfile->getDescriptors().end() )
            {
              continue;
            }

          /* Detect changes.
           * We ignore `group', `priority', `optional', and `systems'. */
          if ( ( desc.name != oldDesc->second.name )
               || ( desc.path != oldDesc->second.path )
               || ( desc.version != oldDesc->second.version )
               || ( desc.semver != oldDesc->second.semver )
               || ( desc.subtree != oldDesc->second.subtree )
               || ( desc.input != oldDesc->second.input ) )
            {
              continue;
            }

          // XXX: Detecting changed groups needs to be handled somewhere.

          sysPkgs.emplace(
            iid,
            this->oldLockfile->getLockfileRaw().packages.at( system ).at(
              iid ) );
        }
      this->lockfileRaw.packages.emplace( system, std::move( sysPkgs ) );
    }
}


/* -------------------------------------------------------------------------- */

// const ManifestRaw::Options &
// Environment::getCombinedOptions()
//{
//   // TODO
// }


/* -------------------------------------------------------------------------- */

const pkgdb::PkgQueryArgs &
Environment::getCombinedBaseQueryArgs()
{
  // TODO: Use `this->getCombinedOptions()'
  if ( ! this->combinedBaseQueryArgs.has_value() )
    {
      this->combinedBaseQueryArgs = this->getManifest().getBaseQueryArgs();
      if ( this->getGlobalManifest().has_value() )
        {
          auto global = this->getGlobalManifest()->getBaseQueryArgs();
          if ( ( ! this->combinedBaseQueryArgs->licenses.has_value() )
               && global.licenses.has_value() )
            {
              this->combinedBaseQueryArgs->licenses = global.licenses;
            }

          /* NOTE: We intentionally skip `systems'. */

          // FIXME: you need to use `getCombinedOptions()' because we have
          //        loss of detail on our `bool` fields.
        }
    }
  return *this->combinedBaseQueryArgs;
}


/* -------------------------------------------------------------------------- */

std::optional<pkgdb::row_id>
Environment::tryResolveDescriptorIn( const ManifestDescriptor & descriptor,
                                     const pkgdb::PkgDbInput &  input,
                                     const System &             system )
{
  /** Skip unrequested systems. */
  if ( descriptor.systems.has_value()
       && ( std::find( descriptor.systems->begin(),
                       descriptor.systems->end(),
                       system )
            == descriptor.systems->end() ) )
    {
      return std::nullopt;
    }

  pkgdb::PkgQueryArgs args = this->getCombinedBaseQueryArgs();
  input.fillPkgQueryArgs( args );
  descriptor.fillPkgQueryArgs( args );
  /* Limit results to the target system. */
  args.systems = std::vector<System> { system };
  pkgdb::PkgQuery query( args );
  auto            rows = query.execute( input.getDbReadOnly()->db );
  if ( rows.empty() ) { return std::nullopt; }
  return rows.front();
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
