/* ========================================================================== *
 *
 * @file flox/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <cstdio>
#include <functional>
#include <optional>
#include <vector>
#include <map>
#include <algorithm>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>


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

  static bool
operator==( const std::list<std::string>      & lhs
          , const std::list<std::string_view> & rhs
          )
{
  return ( lhs.size() == rhs.size() ) && std::equal(
    lhs.cbegin(), lhs.cend(), rhs.cbegin()
  );
}

  static bool
operator==( const std::list<std::string_view> & lhs
          , const std::list<std::string>      & rhs
          )
{
  return ( lhs.size() == rhs.size() ) && std::equal(
    lhs.cbegin(), lhs.cend(), rhs.cbegin()
  );
}


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/** Systems to resolve/search in. */
static const std::list<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

/** `flake' subtrees to resolve/search in. */
static const std::vector<std::string> defaultSubtrees = {
  "catalog", "packages", "legacyPackages"
};

/** Catalog stabilities to resolve/search in. */
static const std::vector<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};


/* -------------------------------------------------------------------------- */

/**
 * Predicate which checks to see if a string is a `flake' "subtree" name.
 * @return true iff `attrName` is one of "legacyPackages", "packages",
 *         or "catalog".
 */
  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "legacyPackages" ) ||
         ( attrName == "packages" )       ||
         ( attrName == "catalog"  );
}


/* -------------------------------------------------------------------------- */

/** `nix' configuration options used when locking flakes. */
static nix::flake::LockFlags floxFlakeLockFlags = {
  .updateLockFile = false
, .writeLockFile  = false
, .applyNixConfig = false
};


/* -------------------------------------------------------------------------- */

/**
 * Predicate which indicates whether a `storePath' is "substitutable".
 * @param storePath an absolute path in the `/nix/store'.
 *        This should be an `outPath' and NOT a `drvPath' in most cases.
 * @return true iff `storePath' is cached in a remote `nix' store and can be
 *              copied without being "rebuilt" from scratch.
 */
bool isSubstitutable( std::string_view storePath );


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
