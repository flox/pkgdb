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

#include <nlohmann/json.hpp>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Perform one time `nix` global runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
void initNix();


/* -------------------------------------------------------------------------- */

/**
 * Runtime state containing a `nix` store connection and a `nix` evaluator.
 */
struct NixState {

/* -------------------------------------------------------------------------- */

  public:

    std::shared_ptr<nix::Store>     store;  /**< Nix store connection.   */
    std::shared_ptr<nix::EvalState> state;  /**< Nix evaluator instance. */


/* -------------------------------------------------------------------------- */

  // public:

    /**
     * Construct `NixState` from an existing store connection.
     * This may be useful if you wish to avoid a non-default store.
     * @param store An open `nix` store connection.
     * @param verbosity Verbosity level setting used throughout `nix` and
     *                  `flox`/`pkgdb` operations.
     */
    explicit NixState( nix::ref<nix::Store> & store
                     , nix::Verbosity         verbosity = nix::lvlInfo
                     )
      : store( (std::shared_ptr<nix::Store>) store )
    {
      nix::verbosity = verbosity;
      initNix();
    }

    /**
     * Construct `NixState` using the systems default `nix` store.
     * @param verbosity Verbosity level setting used throughout `nix` and
     *                  `flox`/`pkgdb` operations.
     */
    explicit NixState( nix::Verbosity verbosity = nix::lvlInfo )
    {
      nix::verbosity = verbosity;
      initNix();
    }


/* -------------------------------------------------------------------------- */

  // public:

    /**
     * Lazily open a `nix` store connection.
     * Connection remains open for lifetime of object.
     */
      nix::ref<nix::Store>
    getStore()
    {
      if ( this->store == nullptr )
        {
          this->store = nix::openStore();
        }
      return (nix::ref<nix::Store>) this->store;
    }


    /**
     * Lazily open a `nix` evaluator.
     * Evaluator remains open for lifetime of object.
     */
      nix::ref<nix::EvalState>
    getState()
    {
      if ( this->state == nullptr )
        {
          this->state = std::make_shared<nix::EvalState>(
            std::list<std::string> {}
          , this->getStore()
          , this->getStore()
          );
          this->state->repair = nix::NoRepair;
        }
      return (nix::ref<nix::EvalState>) this->state;
    }


/* -------------------------------------------------------------------------- */

};  /* End class `NixState' */


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */