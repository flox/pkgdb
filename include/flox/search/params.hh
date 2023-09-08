/* ========================================================================== *
 *
 * @file flox/search/params.hh
 *
 * @brief A set of user inputs used to set input preferences and query
 * parameters during search.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <vector>
#include <map>

#include <nlohmann/json.hpp>
#include <nix/flake/flakeref.hh>
#include <nix/fetchers.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

/**
 * @brief A set of query parameters.
 * This is essentially a reorganized form of @a flox::pkgdb::PkgQueryArgs
 * that is suited for JSON input.
 */
struct SearchQuery : pkgdb::PkgDescriptorBase {

  /** Filter results by partial name/description match. */
  std::optional<std::string> match;

  /** Reset to default state. */
    inline void
  clear()
  {
    this->pkgdb::PkgDescriptorBase::clear();
    this->match = std::nullopt;
  }


  /**
   * Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to lookup
   * packages filtered by @a SearchQuery requirements.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
    pkgdb::PkgQueryArgs &
  fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;

};  /* End struct "SearchQuery' */


/* -------------------------------------------------------------------------- */

void from_json( const nlohmann::json & jfrom,       SearchQuery & qry );
void to_json(         nlohmann::json & jto,   const SearchQuery & qry );


/* -------------------------------------------------------------------------- */

/**
 * @brief SearchParams used to search for packages in a collection of inputs.
 *
 * Example Parameters:
 * ```
 * {
 *   "registry": {
 *     "inputs": {
 *       "nixpkgs": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "NixOS"
 *         , "repo": "nixpkgs"
 *         }
 *       , "subtrees": ["legacyPackages"]
 *       }
 *     , "floco": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "aakropotkin"
 *         , "repo": "floco"
 *         }
 *       , "subtrees": ["packages"]
 *       }
 *     , "floxpkgs": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "flox"
 *         , "repo": "floxpkgs"
 *         }
 *       , "subtrees": ["catalog"]
 *       , "stabilities": ["stable"]
 *       }
 *     }
 *   , "defaults": {
 *       "subtrees": null
 *     , "stabilities": ["stable"]
 *     }
 *   , "priority": ["nixpkgs", "floco", "floxpkgs"]
 *   }
 * , "systems": ["x86_64-linux"]
 * , "allow":   { "unfree": true, "broken": false, "licenses": ["MIT"] }
 * , "semver":  { "preferPreReleases": false }
 * , "query":   { "match": "hello" }
 * }
 * ```
 */
struct SearchParams {

/* -------------------------------------------------------------------------- */

  /** @brief Settings and fetcher information associated with inputs. */
  RegistryRaw registry;

  /**
   * Ordered list of systems to be searched.
   * Results will be grouped by system in the order they appear here.
   */
  std::vector<std::string> systems;


  /** @brief Allow/disallow packages with certain metadata. */
  struct Allows {

    /** Whether to include packages which are explicitly marked `unfree`. */
    bool unfree = true;

    /** Whether to include packages which are explicitly marked `broken`. */
    bool broken = false;

    /** Filter results to those explicitly marked with the given licenses. */
    std::optional<std::vector<std::string>> licenses;

  } allow;


  /** @brief Settings associated with semantic version processing. */
  struct Semver {

    /** Whether pre-release versions should be ordered before releases. */
    bool preferPreReleases = false;

  } semver;


  SearchQuery query;


/* -------------------------------------------------------------------------- */

  /** Reset preferences to default/empty state. */
  void clear();

  /**
   * Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to lookup
   * packages in a particular input.
   * @param input The input name to be searched.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs & fillPkgQueryArgs( const std::string         & input
                                        ,       pkgdb::PkgQueryArgs & pqa
                                        ) const;


};  /* End struct `SearchParams' */


/* -------------------------------------------------------------------------- */

void from_json( const nlohmann::json & jfrom,       SearchParams & params );
void to_json(         nlohmann::json & jto,   const SearchParams & params );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
