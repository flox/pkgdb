/* ========================================================================== *
 *
 * @file flox/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <stdlib.h>
#include <list>
#include <filesystem>
#include <assert.h>
#include <optional>
#include <memory>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "flox/types.hh"
#include "flox/util.hh"
#include "flox/flox-flake.hh"
#include "pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace command {

/* -------------------------------------------------------------------------- */

/** Add verbosity flags to any parser and modify the global verbosity. */
struct VerboseParser : public argparse::ArgumentParser {
  explicit VerboseParser( std::string name, std::string version = "0.1.0" );
};  /* End struct `VerboseParser' */


/* -------------------------------------------------------------------------- */

struct FloxFlakeMixin : virtual public flox::NixState {

  std::unique_ptr<flox::FloxFlake> flake;

  void                 parseFloxFlake( const std::string & arg );
  argparse::Argument & addFlakeRefArg( argparse::ArgumentParser & parser );
};  /* End struct `FloxFlakeMixin' */


/* -------------------------------------------------------------------------- */

/** Adds a package database path to a state blob. */
struct DbPathMixin : virtual public flox::NixState {

  std::optional<std::filesystem::path> dbPath;

    argparse::Argument &
  addDatabasePathOption( argparse::ArgumentParser & parser );

};  /* End struct `DbPathMixin' */


/* -------------------------------------------------------------------------- */

/**
 * Adds a package database and optionally an associated flake to a state blob.
 */
struct PkgDbMixin : virtual public DbPathMixin, virtual public FloxFlakeMixin {

  std::unique_ptr<flox::pkgdb::PkgDb> db;

  void openPkgDb();

  /**
   * Add `target` argument to any parser to read either a `flake-ref` or
   * path to an existing database.
   */
  argparse::Argument & addTargetArg( argparse::ArgumentParser & parser );

};  /* End struct `PkgDbMixin' */


/* -------------------------------------------------------------------------- */

struct AttrPathMixin {

  flox::AttrPath attrPath;

  /* Sets the attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable".
   */
  argparse::Argument & addAttrPathArgs( argparse::ArgumentParser & parser );

  void postProcessArgs();

};  /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

/** Adds optional `-f,--force` flag with associated variable to a state blob. */
struct ForceMixin {

  bool force = false;

    argparse::Argument &
  addForceFlag( argparse::ArgumentParser & parser )
  {
    return parser.add_argument( "-f", "--force" )
                 .help( "Force re-evaluation of prefix" )
                 .nargs( 0 )
                 .action( [&]( const auto & ) { this->force = true; } );
  }

};  /* End struct `ForceMixin' */


/* -------------------------------------------------------------------------- */

struct ScrapeCommand
  : public PkgDbMixin
  , public AttrPathMixin
  , public ForceMixin
{
  VerboseParser parser;

  ScrapeCommand() : flox::NixState(), parser( "scrape" )
  {
    this->parser.add_description( "Scrape a flake and emit a SQLite3 DB" );
    this->addDatabasePathOption( this->parser );
    this->addForceFlag( this->parser );
    this->addFlakeRefArg( this->parser );
    this->addAttrPathArgs( this->parser );
  }

    void
  postProcessArgs()
  {
    static bool didPost = false;
    if ( didPost ) { return; }
    this->AttrPathMixin::postProcessArgs();
    this->openPkgDb();
    didPost = true;
  }

  int run();

};  /* End struct `ScrapeEnv' */


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
