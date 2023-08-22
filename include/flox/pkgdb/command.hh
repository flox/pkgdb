/* ========================================================================== *
 *
 * @file flox/pkgdb/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/pkgdb.hh"
#include "flox/core/command.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

class PkgDb;

/* -------------------------------------------------------------------------- */

/** Adds a package database path to a state blob. */
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
 * Adds a package database and optionally an associated flake to a state blob.
 */
struct PkgDbMixin
  : virtual public DbPathMixin
  , virtual public command::FloxFlakeMixin
{

  std::unique_ptr<flox::pkgdb::PkgDb> db;

  /**
   * Open a @a flox::pkgdb::PkgDb connection using the command state's
   * @a dbPath or @a flake value.
   */
  void openPkgDb();
  inline void postProcessArgs() override { this->openPkgDb(); }

  /**
   * Add `target` argument to any parser to read either a `flake-ref` or
   * path to an existing database.
   */
  argparse::Argument & addTargetArg( argparse::ArgumentParser & parser );

};  /* End struct `PkgDbMixin' */


/* -------------------------------------------------------------------------- */

/** Scrape a flake prefix producing a SQLite3 database with package metadata. */
struct ScrapeCommand : public PkgDbMixin, public command::AttrPathMixin {
  command::VerboseParser parser;

  bool force = false;  /**< Whether to force re-evaluation. */

  ScrapeCommand();

  /** Invoke "child" `preProcessArgs` for `PkgDbMixin` and `AttrPathMixin`. */
  void postProcessArgs() override;

  /**
   * Execute the `scrape` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

/**
 * Minimal set of DB queries, largely focused on looking up info that is
 * non-trivial to query with a "plain" SQLite statement.
 * This subcommand has additional subcommands:
 * - `pkgdb get id [--pkg] DB-PATH ATTR-PATH...`
 *   + Lookup `(AttrSet|Packages).id` for `ATTR-PATH`.
 * - `pkgdb get path [--pkg] DB-PATH ID`
 *   + Lookup `AttrPath` for `(AttrSet|Packages).id`.
 * - `pkgdb get flake DB-PATH`
 *   + Dump the `LockedFlake` table including fingerprint, locked-ref, etc.
 * - `pkgdb get db FLAKE-REF`
 *   + Print the absolute path to the associated flake's db.
 */
struct GetCommand : public PkgDbMixin, public command::AttrPathMixin {
  command::VerboseParser parser;  /**< `get`       parser */
  command::VerboseParser pId;     /**< `get id`    parser  */
  command::VerboseParser pPath;   /**< `get path`  parser */
  command::VerboseParser pFlake;  /**< `get flake` parser */
  command::VerboseParser pDb;     /**< `get db`    parser */
  bool                   isPkg  = false;
  row_id                 id     = 0;

  GetCommand();

  /** Prevent "child" `preProcessArgs` routines from running */
  void postProcessArgs() override {}

  /**
   * Execute the `get id` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runId();

  /**
   * Execute the `get path` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runPath();

  /**
   * Execute the `get flake` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runFlake();

  /**
   * Execute the `get db` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runDb();

  /**
   * Execute the `get` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `GetCommand' */


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */