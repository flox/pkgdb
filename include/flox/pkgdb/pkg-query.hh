/* ========================================================================== *
 *
 * @file flox/pkgdb/pkg-query.hh
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nix/shared.hh>
#include <nlohmann/json.hpp>
#include <sqlite3pp.hh>

#include "compat/concepts.hh"
#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

using row_id = uint64_t; /**< A _row_ index in a SQLite3 table. */


/* -------------------------------------------------------------------------- */

/** @brief Minimal set of query parameters related to a single package. */
struct PkgDescriptorBase
{

  std::optional<std::string> name;    /**< Filter results by exact `name`. */
  std::optional<std::string> pname;   /**< Filter results by exact `pname`. */
  std::optional<std::string> version; /**< Filter results by exact version. */
  std::optional<std::string> semver;  /**< Filter results by version range. */


  /* Base struct boilerplate */
  PkgDescriptorBase()                            = default;
  PkgDescriptorBase( const PkgDescriptorBase & ) = default;
  PkgDescriptorBase( PkgDescriptorBase && )      = default;

  virtual ~PkgDescriptorBase() = default;

  PkgDescriptorBase &
  operator=( const PkgDescriptorBase & )
    = default;
  PkgDescriptorBase &
  operator=( PkgDescriptorBase && )
    = default;

  /** @brief Reset to default state. */
  virtual void
  clear();


}; /* End struct `PkgDescriptorBase' */


/**
 * @fn void from_json( const nlohmann::json & j, PkgDescriptorBase & pdb )
 * @brief Convert a JSON object to a @a flox::pkgdb::PkgDescriptorBase.
 *
 * @fn void to_json( nlohmann::json & j, const PkgDescriptorBase & pdb )
 * @brief Convert a @a flox::pkgdb::PkgDescriptorBase to a JSON object.
 */
/* Generate `to_json' and `from_json' functions. */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT( PkgDescriptorBase,
                                                 name,
                                                 pname,
                                                 version,
                                                 semver )


/**
 * @brief A concept that checks if a typename is derived
 *        from @a flox::pkgdb::PkgDescriptorBase.
 */
template<typename T>
concept pkg_descriptor_typename = std::derived_from<T, PkgDescriptorBase>;


/* -------------------------------------------------------------------------- */

/**
 * @brief Collection of query parameters used to lookup packages in a database.
 *
 * These use a combination of SQL statements and post processing with
 * `node-semver` to produce a list of satisfactory packages.
 */
struct PkgQueryArgs : public PkgDescriptorBase
{

  /* From `PkgDescriptorBase':
   *   std::optional<std::string> name;
   *   std::optional<std::string> pname;
   *   std::optional<std::string> version;
   *   std::optional<std::string> semver;
   */

  /** Filter results by partial match on pname, pkgAttrName, or description */
  std::optional<std::string> partialMatch;

  /**
   * Filter results by an exact match on either `pname` or `pkgAttrName`.
   * To match just `pname` see @a flox::pkgdb::PkgDescriptorBase.
   */
  std::optional<std::string> pnameOrPkgAttrName;

  /** Filter results to those explicitly marked with the given licenses. */
  std::optional<std::vector<std::string>> licenses;

  /** Whether to include packages which are explicitly marked `broken`. */
  bool allowBroken = false;

  /** Whether to include packages which are explicitly marked `unfree`. */
  bool allowUnfree = true;

  /** Whether pre-release versions should be ordered before releases. */
  bool preferPreReleases = false;

  /** Subtrees to search. */
  std::optional<std::vector<Subtree>> subtrees;

  /** Systems to search */
  std::vector<std::string> systems = { nix::settings.thisSystem.get() };

  /** Stabilities to search ( if any ) */
  std::optional<std::vector<std::string>> stabilities;

  /**
   * Relative attribute path to package from its prefix.
   * For catalogs this is the part following `stability`, and for regular flakes
   * it is the part following `system`.
   */
  std::optional<flox::AttrPath> relPath;


  /** @brief Errors concerning validity of package query parameters. */
  class InvalidPkgQueryArgException : public FloxException
  {

  public:

    enum error_code {
      /** Name/{pname,version,semver} are mutually exclusive */
      PQEC_MIX_NAME = 2
      /** Version/semver are mutually exclusive */
      ,
      PQEC_MIX_VERSION_SEMVER = 3,
      PQEC_INVALID_SEMVER     = 4 /**< Semver Parse Error */
      ,
      PQEC_INVALID_LICENSE = 5 /**< License has invalid character */
      ,
      PQEC_INVALID_SUBTREE = 6 /**< Unrecognized subtree */
      ,
      PQEC_CONFLICTING_SUBTREE = 7 /**< Conflicting subtree/stability */
      ,
      PQEC_INVALID_SYSTEM = 8 /**< Unrecognized/unsupported system */
      ,
      PQEC_INVALID_STABILITY = 9 /**< Unrecognized stability */
      ,
      PQEC_INVALID_MATCH_STYLE = 10 /**< `match` without `matchStyle` */
    } errorCode;


  private:

    static std::string
    errorMessage( const error_code & ecode );


  public:

    explicit InvalidPkgQueryArgException( const error_code & ecode )
      : flox::FloxException(
        "encountered an error processing query arguments:",
        InvalidPkgQueryArgException::errorMessage( ecode ) )
      , errorCode( ecode )
    {}

    [[nodiscard]] flox::error_category
    getErrorCode() const noexcept override
    {
      return EC_INVALID_PKG_QUERY_ARG;
    }

    [[nodiscard]] std::string_view
    getCategoryMessage() const noexcept override
    {
      return "encountered an error processing query arguments";
    }


  }; /* End class `InvalidPkgQueryArgException' */


  /** @brief Reset argset to its _default_ state. */
  void
  clear() override;

  /**
   * @brief Sanity check parameters.
   *
   * Make sure `systems` are valid systems.
   * Make sure `stabilities` are valid stabilities.
   * Make sure `name` is not set when `pname`, `version`, or `semver` are set.
   * Make sure `version` is not set when `semver` is set.
   * @return `std::nullopt` iff the above conditions are met, an error
   *         code otherwise.
   */
  [[nodiscard]] std::optional<InvalidPkgQueryArgException::error_code>
  validate() const;

}; /* End struct `PkgQueryArgs' */


/* -------------------------------------------------------------------------- */

/**
 * @brief A query used to lookup packages in a database.
 *
 * This uses a combination of SQL statements and post processing with
 * `node-semver` to produce a list of satisfactory packages.
 */
class PkgQuery : public PkgQueryArgs
{

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
   * @brief Clear member @a PkgQuery member variables of any state from past
   *        initialization runs.
   *
   * This is called by @a init before translating
   * @a flox::pkgdb::PkgQueryArgs members.
   */
  void
  clearBuilt();

  /**
   * @brief Add a new column to the _inner_ `SELECT` statement.
   *
   * These selections may be used internally for filtering and ordering rows,
   * and are only _exported_ in the final result if they are also listed
   * in @a exportedColumns.
   * @param column A column `SELECT` statement such as `v_PackagesSearch.id`
   *               or `0 AS foo`.
   */
  void
  addSelection( std::string_view column );

  /** @brief Appends the `ORDER BY` block. */
  void
  addOrderBy( std::string_view order );

  /**
   * @brief Appends the `WHERE` block with a new `AND ( <COND> )` statement.
   */
  void
  addWhere( std::string_view cond );

  /**
   * @brief Filter a set of semantic version numbers by the range indicated in
   *        the @a semvers member variable.
   *
   * If @a semvers is unset, return the original set _as is_.
   */
  [[nodiscard]] std::unordered_set<std::string>
  filterSemvers( const std::unordered_set<std::string> & versions ) const;

  /** @brief A helper of @a init() which handles `match` filtering/ranking. */
  void
  initMatch();

  /**
   * @brief A helper of @a init() which handles `subtrees` filtering/ranking.
   */
  void
  initSubtrees();

  /**
   * @brief A helper of @a init() which handles `systems` filtering/ranking.
   */
  void
  initSystems();

  /**
   * @brief A helper of @a init() which handles
   *        `stabilities` filtering/ranking.
   */
  void
  initStabilities();

  /** @brief A helper of @a init() which constructs the `ORDER BY` block. */
  void
  initOrderBy();

  /**
   * @brief Translate @a floco::pkgdb::PkgQueryArgs parameters to a _built_
   *        SQL statement held in `std::stringstream` member variables.
   *
   * This is called by constructors, and should be called manually if any
   * @a flox::pkgdb::PkgQueryArgs members are manually edited.
   */
  void
  init();


public:

  PkgQuery() { this->init(); }

  explicit PkgQuery( const PkgQueryArgs & params ) : PkgQueryArgs( params )
  {
    this->init();
  }

  PkgQuery( const PkgQueryArgs &     params,
            std::vector<std::string> exportedColumns )
    : PkgQueryArgs( params ), exportedColumns( std::move( exportedColumns ) )
  {
    this->init();
  }

  /**
   * @brief Produce an unbound SQL statement from various member variables.
   *
   * This must be run after @a init().
   * The returned string still needs to be processed to _bind_ host parameters
   * from @a binds before being executed.
   * @return An unbound SQL query string.
   */
  [[nodiscard]] std::string
  str() const;

  /**
   * @brief Create a bound SQLite query ready for execution.
   *
   * This does NOT perform filtering by `semver` which must be performed as a
   * post-processing step.
   * Unlike @a execute() this routine allows the caller to iterate over rows.
   */
  [[nodiscard]] std::shared_ptr<sqlite3pp::query>
  bind( sqlite3pp::database & pdb ) const;

  /**
   * @brief Query a given database returning an ordered list of
   *        satisfactory `Packages.id`s.
   *
   * This performs `semver` filtering.
   */
  [[nodiscard]] std::vector<row_id>
  execute( sqlite3pp::database & pdb ) const;


}; /* End class `PkgQuery' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
