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
 * @brief Perform one time `nix` global runtime setup.
 *
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
void initNix();


/* -------------------------------------------------------------------------- */

/**
 * @brief Runtime state containing a `nix` store connection and a
 *        `nix` evaluator.
 */
struct NixState {

/* -------------------------------------------------------------------------- */

  public:

    std::shared_ptr<nix::Store>     store;  /**< `nix` store connection.   */
    std::shared_ptr<nix::EvalState> state;  /**< `nix` evaluator instance. */


/* -------------------------------------------------------------------------- */

  // public:

    /**
     * @brief Construct `NixState` from an existing store connection.
     *
     * This may be useful if you wish to avoid a non-default store.
     * @param store An open `nix` store connection.
     * @param verbosity Verbosity level setting used throughout `nix` and
     *                  `flox`/`pkgdb` operations.
     */
    explicit NixState( nix::ref<nix::Store> & store
                     , nix::Verbosity         verbosity = nix::lvlInfo
                     )
      : store( static_cast<std::shared_ptr<nix::Store>>( store ) )
    {
      nix::verbosity = verbosity;
      initNix();
    }

    /**
     * @brief Construct `NixState` using the systems default `nix` store.
     *
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
     * @brief Lazily open a `nix` store connection.
     *
     * Connection remains open for lifetime of object.
     */
      nix::ref<nix::Store>
    getStore()
    {
      if ( this->store == nullptr )
        {
          this->store = nix::openStore();
        }
      return static_cast<nix::ref<nix::Store>>( this->store );
    }


    /**
     * @brief Lazily open a `nix` evaluator.
     *
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
      return static_cast<nix::ref<nix::EvalState>>( this->state );
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
