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

/** Interfaces used to search for packages in flakes. */
namespace flox::search {

/* -------------------------------------------------------------------------- */

/** Package query parser. */
struct PkgQueryMixin {

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
 * @brief Provides a registry of @a PkgDb managers.
 *
 * Derived classes must provide their own @a getRegistryRaw and @a getSystems
 * implementations to support @a initRegistry and @a scrapeIfNeeded.
 */
struct PkgDbRegistryMixin : virtual public NixState {

  protected:

    bool force = false;  /**< Whether to force re-evaluation of flakes. */

    std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;

    /** Initialize @a registry member from @a params.registry. */
    void initRegistry();

    /**
     * Lazily perform scraping on input flakes.
     * If scraping is necessary temprorary read/write handles are opened for
     * those flakes and closed before returning from this function.
     */
    void scrapeIfNeeded();

    /** @return A raw registry used to initialize. */
    [[nodiscard]] virtual RegistryRaw getRegistryRaw() = 0;

    /** @return A list of systems to be scraped. */
    [[nodiscard]] virtual std::vector<std::string> & getSystems() = 0;

};  /* End struct `PkgDbRegistryMixin' */


/* -------------------------------------------------------------------------- */

/** Search flakes for packages satisfying a set of filters. */
class SearchCommand : public PkgDbRegistryMixin, public PkgQueryMixin {

  private:

    SearchParams params;

    /** Add argument to any parser to construct a @a SearchParams. */
      argparse::Argument &
    addSearchParamArgs( argparse::ArgumentParser & parser );


  protected:

      [[nodiscard]]
      virtual RegistryRaw
    getRegistryRaw() override { return this->params.registry; }

      [[nodiscard]]
      virtual std::vector<std::string> &
    getSystems() override { return this->params.systems; }


  public:

    command::VerboseParser parser;

    SearchCommand();

    /** Display a single row from the given @a input. */
    void showRow( std::string_view    inputName
                , pkgdb::PkgDbInput & input
                , pkgdb::row_id       row
                );

    /**
     * Execute the `search` routine.
     * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
     */
    int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
