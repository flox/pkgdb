/* ========================================================================== *
 *
 * @file flox/core/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>  // For `std::hash'
#include <string>      // For `std::string' and `std::string_view'
#include <list>        // For `std::list'

#include <nix/logging.hh>
#include <nix/store-api.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flakeref.hh>

#include <nlohmann/json.hpp>


/* -------------------------------------------------------------------------- */

  template<>
struct std::hash<std::list<std::string_view>>
{
  /**
   * Generate a unique hash for a list of strings.
   * @see flox::resolve::RawPackageMap
   * @param lst a list of strings.
   * @return a unique hash based on the contents of `lst` members.
   */
    std::size_t
  operator()( const std::list<std::string_view> & lst ) const noexcept
  {
    if ( lst.empty() ) { return 0; }
    auto it = lst.begin();
    std::size_t h1 = std::hash<std::string_view>{}( * it );
    for ( ; it != lst.cend(); ++it )
      {
        h1 = ( h1 >> 1 ) ^ ( std::hash<std::string_view>{}( *it ) << 1 );
      }
    return h1;
  }
};


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** Systems to resolve/search in. */
static const std::list<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

/** `flake' subtrees to resolve/search in. */
static const std::list<std::string> defaultSubtrees = {
  "catalog", "packages", "legacyPackages"
};

/** Catalog stabilities to resolve/search in. */
static const std::list<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};


/* -------------------------------------------------------------------------- */

#if 0
// TODO: Migrate from `github:flox/resolver'
/**
 * Predicate which indicates whether a `storePath' is "substitutable".
 * @param storePath an absolute path in the `/nix/store'.
 *        This should be an `outPath' and NOT a `drvPath' in most cases.
 * @return true iff `storePath' is cached in a remote `nix' store and can be
 *              copied without being "rebuilt" from scratch.
 */
bool isSubstitutable( std::string_view storePath );
#endif


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

/**
 * Detect if a path is a SQLite3 database file.
 * @param dbPath Absolute path.
 * @return `true` iff @a path is a SQLite3 database file.
 */
bool isSQLiteDb( const std::string & dbPath );


/* -------------------------------------------------------------------------- */

/**
 * Parse a flake reference from either a JSON attrset or URI string.
 * @param flakeRef JSON or URI string representing a `nix` flake reference.
 * @return Parsed flake reference object.
 */
  inline static nix::FlakeRef
parseFlakeRef( std::string_view flakeRef )
{
  return ( flakeRef.find( '{' ) == flakeRef.npos )
         ? nix::parseFlakeRef( std::string( flakeRef ) )
         : nix::FlakeRef::fromAttrs(
             nix::fetchers::jsonToAttrs( nlohmann::json::parse( flakeRef ) )
           );
}


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
