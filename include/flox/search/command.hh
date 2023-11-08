/* ========================================================================== *
 *
 * @file flox/search/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <vector>

#include "flox/flox-flake.hh"
#include "flox/pkgdb/command.hh"
#include "flox/pkgdb/input.hh"
#include "flox/pkgdb/write.hh"
#include "flox/registry.hh"
#include "flox/resolver/environment.hh"
#include "flox/resolver/lockfile.hh"
#include "flox/resolver/manifest.hh"
#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

/** @brief Interfaces used to search for packages in flakes. */
namespace flox::search {

/* -------------------------------------------------------------------------- */

struct SearchParamsRaw
{

  /**
   * @brief The absolute @a std::filesystem::path to a manifest file or an
   * inline @a flox::resolver::GlobalManifestRaw.
   */
  std::optional<
    std::variant<std::filesystem::path, resolver::GlobalManifestRaw>>
    globalManifestRaw;

  /**
   * @brief The absolute @a std::filesystem::path to a lockfile or an inline
   * @a flox::resolver::LockfileRaw.
   */
  std::optional<std::variant<std::filesystem::path, resolver::LockfileRaw>>
    lockfileRaw;

  /**
   * @brief The @a flox::search::SearchQuery specifying the package to search
   * for.
   */
  SearchQuery query;

public:

  /**
   * @brief Returns the existing @a flox::resolver::LockfileRaw or lazily
   * loads it from disk.
   */
  [[nodiscard]] flox::resolver::LockfileRaw
  getLockfileRaw();

  /**
   * @brief Returns the existing @a flox::resolver::GlobalManifestRaw or lazily
   * loads it from disk.
   */
  [[nodiscard]] flox::resolver::GlobalManifestRaw
  getGlobalManifestRaw();

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages.
   *
   * NOTE: This DOES NOT clear @a pqa before filling it.
   * @param pqa A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs &
  fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;

}; /* End struct `SearchParamsRaw' */

/** @brief Convert a JSON object to a @a flox::search::SearchParamsRaw. */
void
from_json( const nlohmann::json & jfrom, SearchParamsRaw & raw );

/** @brief Convert a @a flox::search::SearchParamsRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const SearchParamsRaw & raw );

/* -------------------------------------------------------------------------- */

/** @brief Search flakes for packages satisfying a set of filters. */
class SearchCommand
  : pkgdb::PkgDbRegistryMixin
  , flox::resolver::EnvironmentMixin
{

private:

  SearchParams           params; /**< Query arguments and inputs */
  command::VerboseParser parser; /**< Query arguments and inputs parser */
  SearchParamsRaw        rawParams;

  /**
   * @brief Add argument to any parser to construct
   *        a @a flox::search::SearchParams.
   */
  argparse::Argument &
  addSearchParamArgs( argparse::ArgumentParser & parser );

  [[nodiscard]] RegistryRaw
  getRegistryRaw() override
  {
    return this->params.registry;
  }

  [[nodiscard]] const std::vector<std::string> &
  getSystems() override
  {
    return this->params.systems;
  }


public:

  SearchCommand();

  /** @brief Display a single row from the given @a input. */
  static void
  showRow( pkgdb::PkgDbInput & input, pkgdb::row_id row )
  {
    std::cout << input.getRowJSON( row ).dump() << std::endl;
  }

  [[nodiscard]] command::VerboseParser &
  getParser()
  {
    return this->parser;
  }

  /**
   * @brief Execute the `search` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int
  run();


}; /* End class `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::search


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
