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

/** @brief Mixin which provides a lazy handle to a `nix` store connection. */
struct NixStoreMixin {

  protected:

    std::shared_ptr<nix::Store> store;  /**< `nix` store connection.   */


  public:

    /**
     * @brief Construct `NixStoreMixin` from an existing store connection.
     *
     * This may be useful if you wish to avoid a non-default store.
     * @param store An open `nix` store connection.
     */
    explicit NixStoreMixin( nix::ref<nix::Store> & store )
      : store( static_cast<std::shared_ptr<nix::Store>>( store ) )
    {
      initNix();
    }

    /**
     * @brief Construct `NixStoreMixin` using the systems default `nix` store.
     */
    NixStoreMixin() { initNix(); }


    /**
     * @brief Lazily open a `nix` store connection.
     *
     * Connection remains open for lifetime of object.
     */
      nix::ref<nix::Store>
    getStore()
    {
      if ( this->store == nullptr ) { this->store = nix::openStore(); }
      return static_cast<nix::ref<nix::Store>>( this->store );
    }


};  /* End struct `NixStoreMixin' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Runtime state containing a `nix` store connection and a
 *        `nix` evaluator.
 */
class NixState : public NixStoreMixin {

  protected:

    /* From `NixStoreMixin':
     *   std::shared_ptr<nix::Store> store
     */

    std::shared_ptr<nix::EvalState> state;  /**< `nix` evaluator instance. */


  public:

    /** @brief Construct `NixState` using the systems default `nix` store. */
    NixState() : NixStoreMixin() {}

    /**
     * @brief Construct `NixState` from an existing store connection.
     *
     * This may be useful if you wish to avoid a non-default store.
     * @param store An open `nix` store connection.
     */
    explicit NixState( nix::ref<nix::Store> & store )
      : NixStoreMixin( store )
    {}


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


};  /* End class `NixState' */


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
