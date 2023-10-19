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
#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

/** @brief Interfaces used to search for packages in flakes. */
namespace flox::search {

/* -------------------------------------------------------------------------- */

/** @brief Package query parser. */
struct PkgQueryMixin {

  pkgdb::PkgQuery query;

  /**
   * @brief Add `query` argument to any parser to construct a
   *        @a flox::pkgdb::PkgQuery.
   */
  argparse::Argument &
  addQueryArgs( argparse::ArgumentParser & parser );

  /**
   * @brief Run query on a @a pkgdb::PkgDbReadOnly database.
   *
   * Any scraping should be performed before invoking this function.
   */
  std::vector<pkgdb::row_id>
  queryDb( pkgdb::PkgDbReadOnly & pdb ) const;


}; /* End struct `PkgQueryMixin' */


/* -------------------------------------------------------------------------- */

/** @brief Search flakes for packages satisfying a set of filters. */
class SearchCommand
  : pkgdb::PkgDbRegistryMixin
  , PkgQueryMixin {

private:
  SearchParams           params; /**< Query arguments and inputs */
  command::VerboseParser parser; /**< Query arguments and inputs parser */

  /**
   * @brief Add argument to any parser to construct
   *        a @a flox::search::SearchParams.
   */
  argparse::Argument &
  addSearchParamArgs( argparse::ArgumentParser & parser );

  [[nodiscard]] RegistryRaw
  getRegistryRaw() override {
    return this->params.registry;
  }

  [[nodiscard]] std::vector<std::string> &
  getSystems() override {
    return this->params.systems;
  }


public:
  SearchCommand();

  /** @brief Display a single row from the given @a input. */
  static void
  showRow( pkgdb::PkgDbInput & input, pkgdb::row_id row ) {
    std::cout << input.getRowJSON( row ).dump() << std::endl;
  }

  [[nodiscard]] command::VerboseParser &
  getParser() {
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
