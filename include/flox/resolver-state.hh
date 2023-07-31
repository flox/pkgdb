/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>
#include <unordered_map>
#include "flox/exceptions.hh"
#include "flox/util.hh"
#include "flox/types.hh"
#include "flox/flox-flake.hh"
#include "flox/resolved.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/**
 * A state blob that holds a `nix' evaluator, an open handle to the `nix' store,
 * open `FloxFlake' instances for the user's inputs, and relevant settings
 * for customizing resolver behavior.
 *
 * Ideally you should only create a single instance of `ResolverState'.
 * If you need to create multiple it is strongly recommended that you close
 * all previously constructed `ResolverState' objects first.
 * This is to avoid synchronization slowdowns in underlying databases.
 */
class ResolverState {
  private:
    std::shared_ptr<nix::Store>                       _store;
    std::shared_ptr<nix::Store>                       evalStore;
    std::shared_ptr<nix::EvalState>                   evalState;
    std::map<std::string, std::shared_ptr<FloxFlake>> _inputs;
    const Preferences                                 _prefs;

  public:

    nix::ref<nix::Store>       getStore();
    nix::ref<nix::Store>       getEvalStore();
    nix::ref<nix::EvalState>   getEvalState();
    nix::SymbolTable         * getSymbolTable();

    ResolverState( const Inputs                 & inputs
                 , const Preferences            & prefs
                 , const std::list<std::string> & systems = defaultSystems
                 );

    Preferences getPreferences() const { return this->_prefs; }

    std::map<std::string, nix::ref<FloxFlake>> getInputs() const;
    std::list<std::string_view>                getInputNames() const;

    std::optional<nix::ref<FloxFlake>> getInput( std::string_view id ) const;

    std::list<Resolved> resolveInInput(       std::string_view   id
                                      , const Descriptor       & desc
                                      );
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
