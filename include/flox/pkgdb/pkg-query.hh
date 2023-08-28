/* ========================================================================== *
 *
 * @file flox/pkgdb/pkg-query.hh
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <sstream>
#include <optional>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include <nix/shared.hh>
#include <sqlite3pp.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

using row_id = uint64_t;  /**< A _row_ index in a SQLite3 table. */


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
        PQEC_ERROR = 1  /**< Generic Exception */
      /** Name/{pname,version,semver} are mutually exclusive */
      , PQEC_MIX_NAME = 2
      /** Version/semver are mutually exclusive */
      , PQEC_MIX_VERSION_SEMVER  = 3
      , PQEC_INVALID_SEMVER      = 4  /**< Semver Parse Error */
      , PQEC_INVALID_LICENSE     = 5  /**< License has invalid character */
      , PQEC_INVALID_SUBTREE     = 6  /**< Unrecognized subtree */
      , PQEC_CONFLICTING_SUBTREE = 7  /**< Conflicting subtree/stability */
      , PQEC_INVALID_SYSTEM      = 8  /**< Unrecognized/unsupported system */
      , PQEC_INVALID_STABILITY   = 9  /**< Unrecognized stability */
      } errorCode;
    protected:
      static std::string errorMessage( const error_code & ec );
    public:
      PkgQueryInvalidArgException( const error_code & ec = PQEC_ERROR )
        : flox::FloxException(
            PkgQueryInvalidArgException::errorMessage( ec )
          )
        , errorCode( ec )
      {}
  };  /* End struct `PkgDbQueryInvalidArgException' */


  /** Reset argset to its _default_ state. */
  void clear();


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


void from_json( const nlohmann::json & j, PkgQueryArgs & pqa );


/* -------------------------------------------------------------------------- */

struct PkgQuery : public PkgQueryArgs {

  std::string from = "v_PackagesSearch";

  std::stringstream selects;
  bool              firstSelect = true;

  std::stringstream orders;
  bool              firstOrder = true;

  std::stringstream wheres;
  bool              firstWhere = true;

  std::unordered_map<std::string, std::string> binds;

  /**
   * Final set of columns to expose after all filtering and ordering has been
   * performed on temporary fields.
   * The value `*` may be used to export all fields.
   *
   * This setting is only intended for use by unit tests, any columns other
   * than `id` and `semver` may be changed without being reflected in normal
   * `pkgdb` semantic version updates.
   */
  std::vector<std::string> exportedColumns = { "id", "semver" };

  /**
   * Add a new column to the _inner_ `SELECT` statement.
   * These selections may be used internally for filtering and ordering rows,
   * and are only _exported_ in the final result if they are also listed
   * in @a exportedColumns.
   * @param column A column `SELECT` statement such as `v_PackagesSearch.id`
   *               or `0 AS foo`.
   */
  void addSelection( std::string_view column );
  /** Appends the `ORDER BY` block. */
  void addOrderBy( std::string_view order );
  /** Appends the `WHERE` block with a new `AND ( <COND> )` statement. */
  void addWhere( std::string_view cond );

  /**
   * Clear member @a PkgQuery member variables of any state from past
   * initialization runs.
   * This is called by @a init before translating @a PkgQueryArgs members.
   */
  void clearBuilt();

  /**
   * Translate @a PkgQueryArgs parameters to a _built_ SQL statement held in
   * `std::stringstream` member variables.
   * This is called by constructors, and should be called manually if any
   * @a PkgQueryArgs members are manually edited.
   */
  void init();

  PkgQuery() { this->init(); }

  PkgQuery( const PkgQueryArgs & params ) : PkgQueryArgs( params )
  {
    this->init();
  }

  PkgQuery( PkgQueryArgs && params ) : PkgQueryArgs( std::move( params ) )
  {
    this->init();
  }

  PkgQuery( const PkgQueryArgs             & params
          , const std::vector<std::string> & exportedColumns
          )
    : PkgQueryArgs( params ), exportedColumns( exportedColumns )
  {
    this->init();
  }

  PkgQuery( PkgQueryArgs             && params
          , std::vector<std::string> && exportedColumns
          )
    : PkgQueryArgs( std::move( params ) )
    , exportedColumns( std::move( exportedColumns ) )
  {
    this->init();
  }

  /**
   * Produce an unbound SQL statement from various member variables.
   * This must be run after @a init.
   * The returned string still needs to be processed to _bind_ host parameters
   * from @a binds before being executed.
   * @return An unbound SQL query string.
   */
  std::string str() const;

  /**
   * Filter a set of semantic version numbers by the range indicated in the
   * @a semvers member variable.
   * If @a semvers is unset, return the original set _as is_.
   */
    std::unordered_set<std::string>
  filterSemvers( const std::unordered_set<std::string> & versions ) const;

  /**
   * Create a bound SQLite query ready for execution.
   * This does NOT perform filtering by `semver` which must be performed as a
   * post-processing step.
   */
  std::shared_ptr<sqlite3pp::query> bind( sqlite3pp::database & db ) const;

  /**
   * Query a given database returning an ordered list of
   * satisfactory `Packages.id`s.
   * This performs `semver` filtering.
   */
  std::vector<row_id> execute( sqlite3pp::database & db ) const;

};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
