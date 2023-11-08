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

/** @brief Search flakes for packages satisfying a set of filters. */
class SearchCommand : flox::resolver::EnvironmentMixin
{

private:

  command::VerboseParser parser; /**< Query arguments and inputs parser */
  SearchParamsRaw        rawParams;

  /**
   * @brief Add argument to any parser to construct
   *        a @a flox::search::SearchParams.
   */
  argparse::Argument &
  addSearchParamArgs( argparse::ArgumentParser & parser );

  /** @brief Convert `SearchParamsRaw' to initialize @a environment. */
  void
  initEnvironment();


public:

  SearchCommand();

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
