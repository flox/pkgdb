/* ========================================================================== *
 *
 * @file flox/pkgdb/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/pkgdb/write.hh"
#include "flox/pkgdb/pkgdb-input.hh"
#include "flox/core/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/** Adds a single package database path to a state blob. */
struct DbPathMixin
  :         public command::CommandStateMixin
  , virtual public flox::NixState
{

  std::optional<std::filesystem::path> dbPath;

  /** Extend an argument parser to accept a `-d,--database PATH` argument. */
    argparse::Argument &
  addDatabasePathOption( argparse::ArgumentParser & parser );

};  /* End struct `DbPathMixin' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Adds a single package database and optionally an associated flake to a
 *        state blob.
 */
  template <pkgdb_typename T>
struct PkgDbMixin
  : virtual public DbPathMixin
  , virtual public command::InlineInputMixin
{

  std::shared_ptr<FloxFlake> flake;
  std::shared_ptr<T>         db;

  /**
   * @brief Open a @a flox::pkgdb::PkgDb connection using the command state's
   *        @a dbPath or @a flake value.
   */
  void openPkgDb();

  inline void postProcessArgs() override { this->openPkgDb(); }

  /**
   * @brief Add `target` argument to any parser to read either a `flake-ref` or
   *        path to an existing database.
   */
  argparse::Argument & addTargetArg( argparse::ArgumentParser & parser );

};  /* End struct `PkgDbMixin' */


/* -------------------------------------------------------------------------- */

/** Scrape a flake prefix producing a SQLite3 database with package metadata. */
struct ScrapeCommand
  : public DbPathMixin
  , public command::AttrPathMixin
  , public command::InlineInputMixin
{

  protected:

    std::optional<PkgDbInput> input;

    bool force = false;  /**< Whether to force re-evaluation. */


  public:

    command::VerboseParser parser;

    ScrapeCommand();

    /** @brief Initialize @a input from @a registryInput. */
    void initInput();

    /**
     * @brief Invoke "child" `preProcessArgs` for `AttrPathMixin`, and
     *        @a initInput.
     */
    void postProcessArgs() override;

    /**
     * @brief Execute the `scrape` routine.
     * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
     */
    int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Minimal set of DB queries, largely focused on looking up info that is
 *        non-trivial to query with a "plain" SQLite statement.
 *
 * This subcommand has additional subcommands:
 * - `pkgdb get id [--pkg] DB-PATH ATTR-PATH...`
 *   + Lookup `(AttrSet|Packages).id` for `ATTR-PATH`.
 * - `pkgdb get done DB-PATH ATTR-PATH...`
 *   + Lookup whether `AttrPath` has been scraped.
 * - `pkgdb get path [--pkg] DB-PATH ID`
 *   + Lookup `AttrPath` for `(AttrSet|Packages).id`.
 * - `pkgdb get flake DB-PATH`
 *   + Dump the `LockedFlake` table including fingerprint, locked-ref, etc.
 * - `pkgdb get db FLAKE-REF`
 *   + Print the absolute path to the associated flake's db.
 */
struct GetCommand
  : public PkgDbMixin<PkgDbReadOnly>
  , public command::AttrPathMixin
{

  command::VerboseParser parser;          /**< `get`       parser */
  command::VerboseParser pId;             /**< `get id`    parser */
  command::VerboseParser pPath;           /**< `get path`  parser */
  command::VerboseParser pDone;           /**< `get done`  parser */
  command::VerboseParser pFlake;          /**< `get flake` parser */
  command::VerboseParser pDb;             /**< `get db`    parser */
  command::VerboseParser pPkg;            /**< `get pkg`   parser */
  bool                   isPkg  = false;
  row_id                 id     = 0;

  GetCommand();

  /** Prevent "child" `postProcessArgs` routines from running */
  void postProcessArgs() override {}

  /**
   * @brief Execute the `get id` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runId();

  /**
   * @brief Execute the `get done` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runDone();

  /**
   * @brief Execute the `get path` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runPath();

  /**
   * @brief Execute the `get flake` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runFlake();

  /**
   * @brief Execute the `get db` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runDb();

  /**
   * @brief Execute the `get pkg` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runPkg();

  /**
   * @brief Execute the `get` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `GetCommand' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
