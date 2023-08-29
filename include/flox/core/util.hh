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
#include <vector>
#include <list>

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
    auto itr = lst.begin();
    std::size_t   hash = std::hash<std::string_view>{}( * itr );
    for ( ; itr != lst.cend(); ++itr )
      {
        hash = ( hash >> ( (unsigned char) 1 ) ) ^
               ( std::hash<std::string_view>{}( * itr ) <<
                 ( (unsigned char) 1 )
               );
      }
    return hash;
  }
};


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** Systems to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultSystems()
{
  static const std::vector<std::string> defaultSystems = {
   "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
  };
  return defaultSystems;
}


/** `flake' subtrees to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultSubtrees()
{
  static const std::vector<std::string> defaultSubtrees = {
    "catalog", "packages", "legacyPackages"
  };
  return defaultSubtrees;
}


/** Catalog stabilities to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultCatalogStabilities()
{
  static const std::vector<std::string> defaultCatalogStabilities = {
    "stable", "staging", "unstable"
  };
  return defaultCatalogStabilities;
}


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
parseFlakeRef( const std::string & flakeRef )
{
  return ( flakeRef.find( '{' ) == std::string::npos )
         ? nix::parseFlakeRef( flakeRef )
         : nix::FlakeRef::fromAttrs(
             nix::fetchers::jsonToAttrs( nlohmann::json::parse( flakeRef ) )
           );
}


/* -------------------------------------------------------------------------- */

/**
 * Parse a JSON object from an inline string or a path to a JSON file.
 * @param jsonOrPath A JSON string or a path to a JSON file.
 * @return A parsed JSON object.
 */
nlohmann::json parseOrReadJSONObject( const std::string & jsonOrPath );


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
