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

/** Minimal set of query parameters related to a single package. */
struct PkgDescriptorBase {
  std::optional<std::string> name;    /**< Filter results by exact `name`. */
  std::optional<std::string> pname;   /**< Filter results by exact `pname`. */
  std::optional<std::string> version; /**< Filter results by exact version. */
  std::optional<std::string> semver;  /**< Filter results by version range. */

  /** Reset to default state. */
    inline void
  clear()
  {
    this->name    = std::nullopt;
    this->pname   = std::nullopt;
    this->version = std::nullopt;
    this->semver  = std::nullopt;
  }
};

void from_json( const nlohmann::json & jfrom,       PkgDescriptorBase & desc );
void to_json(         nlohmann::json & jfrom, const PkgDescriptorBase & desc );


/* -------------------------------------------------------------------------- */

/**
 * Collection of query parameters used to lookup packages in a database.
 * These use a combination of SQL statements and post processing with
 * `node-semver` to produce a list of satisfactory packages.
 */
struct PkgQueryArgs : public PkgDescriptorBase {

  /** Filter results by partial name/description match. */
  std::optional<std::string> match;

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

      static std::string errorMessage( const error_code & ecode );

    public:

      explicit PkgQueryInvalidArgException(
        const error_code & ecode = PQEC_ERROR
      ) : flox::FloxException(
            PkgQueryInvalidArgException::errorMessage( ecode )
          )
        , errorCode( ecode )
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
  [[nodiscard]]
  std::optional<PkgQueryInvalidArgException::error_code> validate() const;

};  /* End struct `PkgQueryArgs' */


void from_json( const nlohmann::json & jqa, PkgQueryArgs & pqa );


/* -------------------------------------------------------------------------- */

/**
 * A query used to lookup packages in a database.
 * This uses a combination of SQL statements and post processing with
 * `node-semver` to produce a list of satisfactory packages.
 */
class PkgQuery : public PkgQueryArgs {

  private:

    /** Stream used to build up the `SELECT` block. */
    std::stringstream selects;
    /** Indicates if @a selects is empty so we know whether to add separator. */
    bool firstSelect = true;

    /** Stream used to build up the `ORDER BY` block. */
    std::stringstream orders;
    /** Indicates if @a orders is empty so we know whether to add separator. */
    bool firstOrder = true;

    /** Stream used to build up the `WHERE` block. */
    std::stringstream wheres;
    /** Indicates if @a wheres is empty so we know whether to add separator. */
    bool firstWhere = true;

    /** `( <PARAM-NAME>, <VALUE> )` pairs that need to be _bound_ by SQLite3. */
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


    /* Member Functions */

    /**
     * Clear member @a PkgQuery member variables of any state from past
     * initialization runs.
     * This is called by @a init before translating @a PkgQueryArgs members.
     */
    void clearBuilt();

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
     * Filter a set of semantic version numbers by the range indicated in the
     * @a semvers member variable.
     * If @a semvers is unset, return the original set _as is_.
     */
      std::unordered_set<std::string>
    filterSemvers( const std::unordered_set<std::string> & versions ) const;

    /** A helper of @a init() which handles `match` filtering/ranking. */
    void initMatch();

    /** A helper of @a init() which handles `subtrees` filtering/ranking. */
    void initSubtrees();

    /** A helper of @a init() which handles `systems` filtering/ranking. */
    void initSystems();

    /** A helper of @a init() which handles `stabilities` filtering/ranking. */
    void initStabilities();

    /** A helper of @a init() which constructs the `ORDER BY` block. */
    void initOrderBy();

    /**
     * Translate @a PkgQueryArgs parameters to a _built_ SQL statement held in
     * `std::stringstream` member variables.
     * This is called by constructors, and should be called manually if any
     * @a PkgQueryArgs members are manually edited.
     */
    void init();


  public:

    PkgQuery() { this->init(); }

    explicit PkgQuery( const PkgQueryArgs & params ) : PkgQueryArgs( params )
    {
      this->init();
    }

    PkgQuery( const PkgQueryArgs             & params
            ,       std::vector<std::string>   exportedColumns
            )
      : PkgQueryArgs( params ), exportedColumns( std::move( exportedColumns ) )
    {
      this->init();
    }

    /**
     * Produce an unbound SQL statement from various member variables.
     * This must be run after @a init().
     * The returned string still needs to be processed to _bind_ host parameters
     * from @a binds before being executed.
     * @return An unbound SQL query string.
     */
    std::string str() const;

    /**
     * Create a bound SQLite query ready for execution.
     * This does NOT perform filtering by `semver` which must be performed as a
     * post-processing step.
     * Unlike @a execute() this routine allows the caller to iterate over rows.
     */
    std::shared_ptr<sqlite3pp::query> bind( sqlite3pp::database & pdb ) const;

    /**
     * Query a given database returning an ordered list of
     * satisfactory `Packages.id`s.
     * This performs `semver` filtering.
     */
    std::vector<row_id> execute( sqlite3pp::database & pdb ) const;

};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
