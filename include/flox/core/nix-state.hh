/* ========================================================================== *
 *
 * @file flox/core/nix-state.hh
 *
 * @brief Manages a `nix` runtime state blob with associated helpers.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nix/logging.hh>
#include <nix/store-api.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flakeref.hh>

#include <nlohmann/json.hpp>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Perform one time `nix` runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
void initNix( nix::Verbosity verbosity = nix::lvlInfo );


/* -------------------------------------------------------------------------- */

/**
 * Runtime state containing a `nix` store connection and a `nix` evaluator.
 */
struct NixState {
  public:
    nix::ref<nix::Store>            store;
    std::shared_ptr<nix::EvalState> state;

    /**
     * Construct `NixState` from an existing store connection.
     * This may be useful if you wish to avoid a non-default store.
     * @param store An open `nix` store connection.
     * @param verbosity Verbosity level setting used throughout `nix` and
     *                  `pkgdb` operations.
     */
    NixState( nix::ref<nix::Store> store
            , nix::Verbosity       verbosity = nix::lvlInfo
            )
      : store( store )
    {
      initNix( verbosity );
      this->state = std::make_shared<nix::EvalState>( std::list<std::string> {}
                                                    , this->store
                                                    , this->store
                                                    );
      this->state->repair = nix::NoRepair;
    }

    /**
     * Construct `NixState` using the systems default `nix` store.
     * @param verbosity Verbosity level setting used throughout `nix` and
     *                  `pkgdb` operations.
     */
    NixState( nix::Verbosity verbosity = nix::lvlInfo )
      : NixState( ( [&]() {
                      initNix( verbosity );
                      return nix::ref<nix::Store>( nix::openStore() );
                    } )()
                , verbosity
                )
    {}

};  /* End class `NixState' */


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
