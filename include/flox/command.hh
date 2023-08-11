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
  /** Executable command helpers, argument parsers, etc. */
  namespace command {

/* -------------------------------------------------------------------------- */

/** Add verbosity flags to any parser and modify the global verbosity. */
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
  bool force = false;    /**< Whether to force re-evaluation. */

  ScrapeCommand() : flox::NixState(), parser( "scrape" )
  {
    this->parser.add_description( "Scrape a flake and emit a SQLite3 DB" );
    this->parser.add_argument( "-f", "--force" )
                .help( "Force re-evaluation of flake" )
                .nargs( 0 )
                .action( [&]( const auto & ) { this->force = true; } );
    this->addDatabasePathOption( this->parser );
    this->addFlakeRefArg( this->parser );
    this->addAttrPathArgs( this->parser );
  }

    void
  postProcessArgs() override
  {
    static bool didPost = false;
    if ( didPost ) { return; }
    this->AttrPathMixin::postProcessArgs();
    this->PkgDbMixin::postProcessArgs();
    didPost = true;
  }

  /**
   * Execute the `scrape` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

struct GetCommand : public PkgDbMixin
{
  VerboseParser       parser;
  VerboseParser       pPath;
  bool                isPkg = false;
  flox::pkgdb::row_id id    = 0;

  GetCommand()
    : flox::NixState()
    , parser( "get" )
    , pPath( "path" )
  {
    this->parser.add_description( "Get metadata from Package DB" );
    this->pPath.add_description(
      "Lookup an (AttrSets|Packages).id attribute path"
    );
    this->pPath.add_argument( "-p", "--pkg" )
               .help( "Lookup `Packages.id'" )
               .nargs( 0 )
               .action( [&]( const auto & ) { this->isPkg = true; } );
    this->addTargetArg( this->pPath );
    this->pPath.add_argument( "id" )
               .help( "Row `id' to lookup" )
               .nargs( 1 )
               .action( [&]( const std::string & i )
                        {
                          this->id = std::stoull( i );
                        }
                      );
    this->parser.add_subparser( this->pPath );
  }

  /**
   * Execute the `get` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
    int
  run()
  {
    if ( this->parser.is_subcommand_used( "path" ) )
      {
        if ( this->isPkg )
          {
            throw flox::FloxException( "TODO: --pkg" );
            return EXIT_FAILURE;
          }
        else
          {
            std::cout << nlohmann::json( this->db->getAttrSetPath( id ) ).dump()
                      << std::endl;
          }
      }
    else
      {
        std::cerr << this->parser << std::endl;
        throw flox::FloxException(
                "You must provide a valid 'get' subcommand"
              );
        return EXIT_FAILURE;
      }
    return EXIT_SUCCESS;
  }

};  /* End struct `GetCommand' */


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
