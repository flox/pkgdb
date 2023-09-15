/* ========================================================================== *
 *
 * @file flox/search/params.hh
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during search.
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
#include "flox/pkgdb/params.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

/**
 * @brief A set of query parameters.
 *
 * This is essentially a reorganized form of @a flox::pkgdb::PkgQueryArgs
 * that is suited for JSON input.
 */
struct SearchQuery : public pkgdb::PkgDescriptorBase {

  /* From `pkgdb::PkgDescriptorBase`:
   *   std::optional<std::string> name;
   *   std::optional<std::string> pname;
   *   std::optional<std::string> version;
   *   std::optional<std::string> semver;
   */

  /** Filter results by partial name/description match. */
  std::optional<std::string> match;


  /** @brief Reset to default state. */
  virtual void clear() override;

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages filtered by @a SearchQuery requirements.
   *
   * NOTE: This DOES NOT clear @a pqa before filling it.
   * This is intended to be used after filling @a pqa with global preferences.
   * @param pqa A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs & fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;

};  /* End struct "SearchQuery' */


/* -------------------------------------------------------------------------- */

/** Convert a JSON object to a @a flox::search::SearchQuery. */
void from_json( const nlohmann::json & jfrom, SearchQuery & qry );

/** Convert a @a flox::search::SearchQuery to a JSON object. */
void to_json( nlohmann::json & jto, const SearchQuery & qry );


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
using SearchParams = pkgdb::QueryParams<SearchQuery>;


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
