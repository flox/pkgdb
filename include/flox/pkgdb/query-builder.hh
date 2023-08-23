/* ========================================================================== *
 *
 * @file flox/pkgdb/query-builder.hh
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <functional>

#include <nix/shared.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/**
 * Collection of query parameters used to lookup packages in a database.
 * These use a combination of SQL statements and post processing with
 * `node-semver` to produce a list of satisfactory packages.
 */
struct PkgQueryArgs {

  /** Filter results by partial name/description match. */
  std::optional<std::string> match;
  std::optional<std::string> name;    /**< Filter results by exact `name`. */
  std::optional<std::string> pname;   /**< Filter results by exact `pname`. */
  std::optional<std::string> version; /**< Filter results by exact version. */
  std::optional<std::string> semver;  /**< Filter results by version range. */

  /** Filter results to those explicitly marked with the given licenses. */
  std::optional<std::vector<std::string>> licenses;

  /** Whether to include packages which are explicitly marked `broken`. */
  bool allowBroken = false;
  /** Whether to include packages which are explicitly marked `unfree`. */
  bool allowUnfree = true;
  /** Whether pre-release versions should be ordered before releases. */
  bool preferPreReleases = false;

  /**
   * Subtrees to search.
   * TODO: Default to first of `catalog`, `packages`, or `legacyPackages`.
   *       Requires `db` to be read.
   */
  std::optional<std::vector<subtree_type>> subtrees;

  /** Systems to search */
  std::vector<std::string> systems = { nix::settings.thisSystem.get() };

  /** Stabilities to search ( if any ) */
  std::optional<std::vector<std::string>> stabilities;


  /** Errors concerning validity of package query parameters. */
  struct PkgQueryInvalidArgException : public flox::FloxException {
    public:
      enum error_code {
        PDQEC_ERROR = 1  /**< Generic Exception */
      /** Name/{pname,version,semver} are mutually exclusive */
      , PDQEC_MIX_NAME = 2
      /** Version/semver are mutually exclusive */
      , PDQEC_MIX_VERSION_SEMVER  = 3
      , PDQEC_INVALID_SEMVER      = 4  /**< Semver Parse Error */
      , PDQEC_INVALID_LICENSE     = 5  /**< License has invalid character */
      , PDQEC_INVALID_SUBTREE     = 6  /**< Unrecognized subtree */
      , PDQEC_CONFLICTING_SUBTREE = 7  /**< Conflicting subtree/stability */
      , PDQEC_INVALID_SYSTEM      = 8  /**< Unrecognized/unsupported system */
      , PDQEC_INVALID_STABILITY   = 9  /**< Unrecognized stability */
      } errorCode;
    protected:
      static std::string errorMessage( const error_code & ec );
    public:
      PkgQueryInvalidArgException( const error_code & ec = PDQEC_ERROR )
        : flox::FloxException(
            PkgQueryInvalidArgException::errorMessage( ec )
          )
        , errorCode( ec )
      {}
  };  /* End struct `PkgDbQueryInvalidArgException' */

  /**
   * Sanity check parameters.
   * Make sure `systems` are valid systems.
   * Make sure `stabilities` are valid stabilities.
   * Make sure `name` is not set when `pname`, `version`, or `semver` are set.
   * Make sure `version` is not set when `semver` is set.
   * @return `std::nullopt` iff the above conditions are met, an error
   *         code otherwise.
   */
  std::optional<PkgQueryInvalidArgException::error_code> validate() const;

};  /* End struct `PkgQueryArgs' */


/* -------------------------------------------------------------------------- */

 /* A SQL statement string with a mapping of host parameters to their
  * respective values. */
using SQLBinds = std::unordered_map<std::string, std::string>;

/**
 * Construct a SQL query string with a set of parameters to be bound.
 * Binding is left to the caller to allow a single result to be reused across
 * multiple databases.
 *
 * This routine does NOT perform filtering by `semver`.
 *
 * @return A SQL statement string with a mapping of host parameters to their
 *         respective values.
 *         The resulting statement selects `id`, `semver`, and an enum field
 *         `matchStrength` holding either `NULL` when no @a match parameter was
 *         given, or 0-3 otherwise associated with @a match_strength values.
 */
std::pair<std::string, SQLBinds> buildPkgQuery( const PkgQueryArgs & params );


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
