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
class ResolverState : protected pkgdb::PkgDbRegistryMixin {

  private:

    RegistryRaw registryRaw;  /**< Flake inputs to resolve in. */

    pkgdb::QueryPreferences preferences;  /**< _Global_ resolution settings. */


  protected:

      [[nodiscard]]
      virtual RegistryRaw
    getRegistryRaw() override
    {
      return this->registryRaw;
    }


      [[nodiscard]]
      virtual std::vector<std::string> &
    getSystems() override
    {
      return this->preferences.systems;
    }


    /**
     * @brief Initializes `PkgDbRegistryMixin::registry` and scrapes inputs
     *        when necessary.
     */
      void
    initResolverState()
    {
      static bool didInit = false;
      if ( ! didInit )
        {
          this->initRegistry();
          this->scrapeIfNeeded();
          didInit = true;
        }
    }


  public:

    /**
     * @brief Construct a new @a ResolverState from a set of
     *        @a flox::resolver::ResolveOneParams.
     *
     * The descriptor member of these preferences is ignored.
     */
    explicit ResolverState( const ResolveOneParams & params )
      : registryRaw( params.registry )
      , preferences( dynamic_cast<const pkgdb::QueryPreferences &>( params ) )
    {}


      [[nodiscard]]
      nix::ref<Registry<pkgdb::PkgDbInputFactory>>
    getPkgDbRegistry()
    {
      this->initResolverState();
      return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>(
        this->registry
      );
    }


};  /* End class `ResolverState' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
