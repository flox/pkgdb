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
        * this->dbFactory );
    }
  return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>( this->dbs );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
