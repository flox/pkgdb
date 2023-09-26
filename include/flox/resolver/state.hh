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

  /* From `PkgDbRegistryMixin':
   *   public:
   *     std::shared_ptr<nix::Store>     store
   *     std::shared_ptr<nix::EvalState> state
   *   protected:
   *     bool                                                force    = false;
   *     std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;
   */

  private:

    RegistryRaw registryRaw;  /**< Flake inputs to resolve in. */

    pkgdb::QueryPreferences preferences;  /**< _Global_ resolution settings. */


  protected:

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
      if ( this->registry == nullptr )
        {
          this->initRegistry();
          this->scrapeIfNeeded();
        }
      assert( this->registry != nullptr );
    }


  public:

    ResolverState( const RegistryRaw             & registry
                 , const pkgdb::QueryPreferences & preferences
                 )
      : registryRaw( registry )
      , preferences( preferences )
    {}


      [[nodiscard]]
      virtual RegistryRaw
    getRegistryRaw() override
    {
      return this->registryRaw;
    }


      [[nodiscard]]
      nix::ref<Registry<pkgdb::PkgDbInputFactory>>
    getPkgDbRegistry()
    {
      this->initResolverState();
      return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>(
        this->registry
      );
    }

      pkgdb::PkgQueryArgs
    getPkgQueryArgs( const std::string & name )
    {
      this->initResolverState();
      pkgdb::PkgQueryArgs args;
      this->preferences.fillPkgQueryArgs( args );
      this->registry->at( name )->fillPkgQueryArgs( args );
      args.matchStyle = pkgdb::PkgQueryArgs::QMS_RESOLVE;
      return args;
    }

};  /* End class `ResolverState' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
