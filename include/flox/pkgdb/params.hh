/* ========================================================================== *
 *
 * @file flox/pkgdb/params.hh
 *
 * @brief User settings used to query a package database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/**
 * @brief Global preferences used for resolution/search with multiple queries.
 */
struct QueryPreferences {

  /**
   * Ordered list of systems to be searched.
   * Results will be grouped by system in the order they appear here.
   */
  std::vector<std::string> systems = { nix::settings.thisSystem.get() };


  /** @brief Allow/disallow packages with certain metadata. */
  struct Allows {

    /** Whether to include packages which are explicitly marked `unfree`. */
    bool unfree = true;

    /** Whether to include packages which are explicitly marked `broken`. */
    bool broken = false;

    /** Filter results to those explicitly marked with the given licenses. */
    std::optional<std::vector<std::string>> licenses;

  };  /* End struct `ResolveOneParams::Allows' */

  Allows allow; /**< Allow/disallow packages with certain metadata. */


  /**
   * @brief Settings associated with semantic version processing.
   *
   * These act as the _global_ default, but may be overridden by
   * individual descriptors.
   */
  struct Semver {
    /** Whether pre-release versions should be ordered before releases. */
    bool preferPreReleases = false;
  };  /* End struct `ResolveOneParams::Semver' */

  Semver semver;  /**< Settings associated with semantic version processing. */


  /** @brief Reset to default/empty state. */
  virtual void clear();

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages.
   *
 * NOTE: This DOES clear @a pqa before filling it.
   */
  pkgdb::PkgQueryArgs & fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;

};  /* End struct `QueryPreferences' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a JSON object to a @a flox::pkgdb::QueryPreferences.
 *
 * NOTE: This DOES clear @a prefs before filling it.
 */
void from_json( const nlohmann::json & jfrom, QueryPreferences & prefs );

/**
 * @brief Convert a @a flox::pkgdb::QueryPreferences to a JSON object.
 *
 * NOTE: This DOES clear @a jto before filling it.
 */
void to_json( nlohmann::json & jto, const QueryPreferences & prefs );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
