/* ========================================================================== *
 *
 * @file flox/search/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <functional>

#include "flox/flox-flake.hh"
#include "flox/pkgdb/write.hh"
#include "flox/pkgdb/command.hh"
#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces used to search for packages in flakes. */
  namespace search {

/* -------------------------------------------------------------------------- */

/** Parse a set of user's _inputs_, being flakes to search in. */
struct InputsMixin
  : virtual public command::CommandStateMixin
  ,         public flox::FloxFlakeParserMixin
{

  /** A flake and its associated package database.  */
  struct Input {
    std::shared_ptr<flox::FloxFlake>      flake;
    std::shared_ptr<pkgdb::PkgDbReadOnly> db;
  };  /* End struct `InputsMixin::Input' */

  /**
   * Mapping of short name aliases to flakes/dbs.
   * A vector of pairs is used to preserve the user's ordering which effects
   * the order of search results.
   */
  std::vector<std::pair<std::string, Input>> inputs;

  /**
   * Parse inputs from an inline JSON string, or JSON file.
   * @a Input::flake members are initialized, but no databases are opened yet.
   */
  void parseInputs( const std::string & jsonOrFile );

  /**
   * @brief Open read-only database handles forall inputs.
   *
   * After parsing query parameters those handles may be changed to read/write
   * if scraping is required, but ownership of writable databases is not handled
   * by this mixin.
   */
  void openDatabases();

  inline void postProcessArgs() override { this->openDatabases(); }

  /** Add `inputs` argument to parse flake inputs from. */
  argparse::Argument & addInputsArg( argparse::ArgumentParser & parser );

};  /* End struct `InputsMixin' */


/* -------------------------------------------------------------------------- */

/** Package query parser. */
struct PkgQueryMixin : virtual public command::CommandStateMixin {

  pkgdb::PkgQuery query;

  /** Add `query` argument to any parser to construct a @a pkgdb::PkgQuery. */
  argparse::Argument & addQueryArgs( argparse::ArgumentParser & parser );

  /**
   * Run query on a @a pkgdb::PkgDbReadOnly database.
   * Any scraping should be performed before invoking this function.
   */
  std::vector<pkgdb::row_id> queryDb( pkgdb::PkgDbReadOnly & pdb ) const;

};  /* End struct `RegistryMixin' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Search parameters _single JSON object_ parser.
 *
 * This uses the same _plumbin_ as @a InputsMixin and @a PkgQueryMixin for
 * post-processing, but does not use their actual parsers.
 */
struct SearchParamsMixin
  : public InputsMixin
  , public PkgQueryMixin
{
  SearchParams params;

  /** Add argument to any parser to construct a @a search::SearchParams. */
  argparse::Argument & addSearchParamArgs( argparse::ArgumentParser & parser );

  /** Sets @a pkgdb::PkgQuery member with settings for a named input. */
  void setInput( const std::string & name );

  using InputsMixin::postProcessArgs;

};  /* End struct `SearchParamsMixin' */


/* -------------------------------------------------------------------------- */

/** Search flakes for packages satisfying a set of filters. */
struct SearchCommand :  SearchParamsMixin
{

  command::VerboseParser parser;

  bool force = false;  /**< Whether to force re-evaluation. */

  SearchCommand();

  /**
   * Lazily perform scraping on input flakes.
   * If scraping is necessary temprorary read/write handles are opened for those
   * flakes and closed before returning from this function.
   */
  void scrapeIfNeeded();

  /**
   * Invoke "child" `preProcessArgs`, and trigger scraping if necessary.
   * This may trigger scraping.
   */
  void postProcessArgs() override;

  /** Display a single row from the given @a input. */
  void showRow(       std::string_view     inputName
              , const InputsMixin::Input & input
              ,       pkgdb::row_id        row
              );

  /**
   * Execute the `search` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::search' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
