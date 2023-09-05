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
#include "flox/pkgdb/pkgdb-input.hh"
#include "flox/search/params.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces used to search for packages in flakes. */
  namespace search {

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
struct SearchParamsMixin : virtual public NixState, public PkgQueryMixin {

  SearchParams params;

  /** Add argument to any parser to construct a @a search::SearchParams. */
  argparse::Argument & addSearchParamArgs( argparse::ArgumentParser & parser );

};  /* End struct `SearchParamsMixin' */


/* -------------------------------------------------------------------------- */

/** Search flakes for packages satisfying a set of filters. */
struct SearchCommand : public SearchParamsMixin
{

  protected:

    bool force = false;  /**< Whether to force re-evaluation. */

    std::shared_ptr<Registry<pkgdb::PkgDbInput>> registry;

    /** Initialize @a registry member from @a params.registry. */
    void initRegistry();

    /**
     * Lazily perform scraping on input flakes.
     * If scraping is necessary temprorary read/write handles are opened for
     * those flakes and closed before returning from this function.
     */
    void scrapeIfNeeded();


  public:

    command::VerboseParser parser;

    SearchCommand();

    /** Display a single row from the given @a input. */
    void showRow( std::string_view    inputName
                , pkgdb::PkgDbInput & input
                , pkgdb::row_id       row
                );

    /**
     * Invoke "child" `preProcessArgs`, and trigger scraping if necessary.
     * This may trigger scraping.
     */
    void postProcessArgs() override;

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
