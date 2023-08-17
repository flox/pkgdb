/* ========================================================================== *
 *
 * @file flox/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <optional>
#include <memory>

#include <argparse/argparse.hpp>

#include "flox/core/types.hh"
#include "flox/flox-flake.hh"

/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb { class PkgDb; };

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

  /**
   * Sets fallback `attrPath` to a package set.
   * If `attrPath` is empty use, `packages.<SYTEM>`.
   * If `attrPath` is one element then add "current system" as `<SYSTEM>`.
   * If `attrPath` is a catalog with no stability use `stable`.
   */
  void postProcessArgs() override;

};  /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
