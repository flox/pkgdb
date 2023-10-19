/* ========================================================================== *
 *
 * @file flox/resolver/state.hh
 *
 * @brief A runtime state used to perform resolution.
 *
 * This comprises a set of inputs with @a flox::pkgdb::PkgDbInput handles and
 * a set of descriptors to be resolved.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once


#include "flox/pkgdb/input.hh"
#include "flox/pkgdb/params.hh"
#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief A runtime state used to perform resolution.
 *
 * This comprises a set of inputs with @a flox::pkgdb::PkgDbInput handles and
 * a set of descriptors to be resolved.
 */
class ResolverState : public pkgdb::PkgDbRegistryMixin
{

private:
  RegistryRaw registryRaw; /**< Flake inputs to resolve in. */

  pkgdb::QueryPreferences preferences; /**< _Global_ resolution settings. */


protected:
  /* From `PkgDbRegistryMixin':
   *   std::shared_ptr<nix::Store>                         store
   *   bool                                                force    = false
   *   std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry
   */

  [[nodiscard]] std::vector<std::string> &
  getSystems() override
  {
    return this->preferences.systems;
  }


public:
  ResolverState( RegistryRaw registry, pkgdb::QueryPreferences preferences )
    : registryRaw( std::move( registry ) )
    , preferences( std::move( preferences ) )
  {}


  /** @brief Get the _raw_ registry declaration. */
  [[nodiscard]] RegistryRaw
  getRegistryRaw() override
  {
    return this->registryRaw;
  }


  /**
   * @brief Get a _base_ set of query arguments for the input associated with
   *        @a name and declared @a preferences.
   */
  [[nodiscard]] pkgdb::PkgQueryArgs
  getPkgQueryArgs( const std::string & name )
  {
    pkgdb::PkgQueryArgs args;
    this->preferences.fillPkgQueryArgs( args );
    this->getPkgDbRegistry()->at( name )->fillPkgQueryArgs( args );
    return args;
  }


}; /* End class `ResolverState' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
