/* ========================================================================== *
 *
 * @file flox/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <optional>
#include <memory>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "flox/types.hh"
#include "flox/flox-flake.hh"
#include "pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  /** Executable command helpers, argument parsers, etc. */
  namespace command {

/* -------------------------------------------------------------------------- */

/**
 * @brief Add verbosity flags to any parser and modify the global verbosity.
 *
 * Nix verbosity levels for reference ( we have no `--debug` flag ):
 *   typedef enum {
 *     lvlError = 0   ( --quiet --quiet --quiet )
 *   , lvlWarn        ( --quiet --quiet )
 *   , lvlNotice      ( --quiet )
 *   , lvlInfo        ( **Default** )
 *   , lvlTalkative   ( -v )
 *   , lvlChatty      ( -vv   | --debug --quiet )
 *   , lvlDebug       ( -vvv  | --debug )
 *   , lvlVomit       ( -vvvv | --debug -v )
 *   } Verbosity;
 */
struct VerboseParser : public argparse::ArgumentParser {
  explicit VerboseParser( std::string name, std::string version = "0.1.0" );
};  /* End struct `VerboseParser' */


/* -------------------------------------------------------------------------- */

/** Virtual class for _mixins_ which extend a command's state blob */
struct CommandStateMixin {
  /** Hook run after parsing arguments and before running commands. */
  virtual void postProcessArgs() {};
};  /* End struct `CommandStateMixin' */


/* -------------------------------------------------------------------------- */

/** Extend a command's state blob with a @a flox::FloxFlake. */
struct FloxFlakeMixin
  :         public CommandStateMixin
  , virtual public flox::NixState
{

  std::unique_ptr<flox::FloxFlake> flake;

  /**
   * Populate the command state's @a flake with the a flake reference.
   * @param flakeRef A URI string or JSON representation of a flake reference.
   */
  void parseFloxFlake( const std::string & flakeRef );

  /** Extend an argument parser to accept a `flake-ref` argument. */
  argparse::Argument & addFlakeRefArg( argparse::ArgumentParser & parser );
};  /* End struct `FloxFlakeMixin' */


/* -------------------------------------------------------------------------- */

/** Adds a package database path to a state blob. */
struct DbPathMixin : public CommandStateMixin, virtual public flox::NixState {

  std::optional<std::filesystem::path> dbPath;

  /** Extend an argument parser to accept a `-d,--database PATH` argument. */
    argparse::Argument &
  addDatabasePathOption( argparse::ArgumentParser & parser );

};  /* End struct `DbPathMixin' */


/* -------------------------------------------------------------------------- */

/**
 * Adds a package database and optionally an associated flake to a state blob.
 */
struct PkgDbMixin : virtual public DbPathMixin, virtual public FloxFlakeMixin {

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

/** Extend a command state blob with an attribute path to "target". */
struct AttrPathMixin : public CommandStateMixin {

  flox::AttrPath attrPath;

  /**
   * Sets the attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable".
   */
  argparse::Argument & addAttrPathArgs( argparse::ArgumentParser & parser );

  void postProcessArgs() override;

};  /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

struct ScrapeCommand : public PkgDbMixin, public AttrPathMixin {
  VerboseParser parser;
  bool          force  = false;    /**< Whether to force re-evaluation. */

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
struct GetCommand : public PkgDbMixin, public AttrPathMixin
{
  VerboseParser       parser;  /**< `get`       parser */
  VerboseParser       pId;     /**< `get id`    parser  */
  VerboseParser       pPath;   /**< `get path`  parser */
  VerboseParser       pFlake;  /**< `get flake` parser */
  VerboseParser       pDb;     /**< `get db`    parser */
  bool                isPkg  = false;
  flox::pkgdb::row_id id     = 0;

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
