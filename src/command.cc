/* ========================================================================== *
 *
 * @file command.cc
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

/** Add verbosity flags to any parser and modify the global verbosity. */
struct VerboseParser : public argparse::ArgumentParser {
  explicit VerboseParser( std::string name, std::string version = "0.1.0" )
    : argparse::ArgumentParser( name, version )
  {
      /* Nix verbosity levels for reference:
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
      this->add_argument( "-q", "--quiet" )
        .help(
          "Decreate the logging verbosity level. May be used up to 3 times."
        ).action(
          [&]( const auto & )
          {
            nix::verbosity =
              ( nix::verbosity <= nix::lvlError )
                ? nix::lvlError : (nix::Verbosity) ( nix::verbosity - 1 );
          }
        ).default_value( false ).implicit_value( true )
        .append();
      this->add_argument( "-v", "--verbose" )
        .help( "Increase the logging verbosity level. May be up to 4 times." )
        .action(
          [&]( const auto & )
          {
            nix::verbosity =
              ( nix::lvlVomit <= nix::verbosity )
                ? nix::lvlVomit : (nix::Verbosity) ( nix::verbosity + 1 );
          }
        ).default_value( false ).implicit_value( true )
        .append();
  }
};  /* End struct `VerboseParser' */


/* -------------------------------------------------------------------------- */

struct FloxFlakeMixin : virtual public flox::NixState {

  std::unique_ptr<flox::FloxFlake> flake;

    void
  parseFloxFlake( const std::string & arg )
  {
    nix::FlakeRef ref =
      ( arg.find( '{' ) == arg.npos ) ? nix::parseFlakeRef( arg )
                                      : nix::FlakeRef::fromAttrs(
                                          nix::fetchers::jsonToAttrs(
                                            nlohmann::json::parse( arg )
                                          )
                                        );
    {
      nix::Activity act(
        * nix::logger
      , nix::lvlInfo
      , nix::actUnknown
      , nix::fmt( "fetching flake '%s'", ref.to_string() )
      );
      this->flake = std::make_unique<flox::FloxFlake>(
        (nix::ref<nix::EvalState>) this->state
      , ref
      );
    }

    if ( ! this->flake->lockedFlake.flake.lockedRef.input.hasAllInfo()
       )
      {
        if ( nix::lvlWarn <= nix::verbosity )
          {
            nix::logger->warn(
              "flake-reference is unlocked/dirty - "
              "resulting DB may not be cached."
            );
          }
      }
  }

    argparse::Argument &
  addFlakeRefArg( argparse::ArgumentParser & parser )
  {
    return parser.add_argument( "flake-ref" )
                 .help(
                   "flake-ref URI string or JSON attrs ( preferably locked )"
                 )
                 .required()
                 .metavar( "FLAKE-REF" )
                 .action( [&]( const std::string & ref )
                          {
                            this->parseFloxFlake( ref );
                          }
                        );
  }

};  /* End struct `FloxFlakeMixin' */


/* -------------------------------------------------------------------------- */

/** Adds a package database path to a state blob. */
struct DbPathMixin : virtual public flox::NixState {

  std::optional<std::filesystem::path> dbPath;

    argparse::Argument &
  addDatabasePathOption( argparse::ArgumentParser & parser )
  {
    return parser.add_argument( "-d", "--database" )
                 .help( "Use database at PATH" )
                 .default_value( "" )
                 .metavar( "PATH" )
                 .nargs( 1 )
                 .action( [&]( const std::string & dbPath )
                          {
                            this->dbPath = nix::absPath( dbPath );
                            std::filesystem::create_directories(
                              this->dbPath.value().parent_path()
                            );
                          }
                        );
  }

};  /* End struct `DbPathMixin' */


/* -------------------------------------------------------------------------- */

/**
 * Adds a package database and optionally an associated flake to a state blob.
 */
struct PkgDbMixin : virtual public DbPathMixin, virtual public FloxFlakeMixin {

  std::unique_ptr<flox::pkgdb::PkgDb> db;

    void
  openPkgDb()
  {
    if ( this->db != nullptr ) { return; }  /* Already loaded. */
    if ( ( this->flake != nullptr ) && this->dbPath.has_value() )
      {
        this->db = std::make_unique<flox::pkgdb::PkgDb>(
          this->flake->lockedFlake
        , (std::string) this->dbPath.value()
        );
      }
    else if ( this->flake != nullptr )
      {
        this->dbPath =
          flox::pkgdb::genPkgDbName( this->flake->lockedFlake );
        this->db = std::make_unique<flox::pkgdb::PkgDb>(
          this->flake->lockedFlake
        , (std::string) this->dbPath.value()
        );
      }
    else if ( this->dbPath.has_value() )
      {
        this->db = std::make_unique<flox::pkgdb::PkgDb>(
          (std::string) this->dbPath.value()
        );
      }
    else
      {
        throw flox::FloxException(
          "You must provide either a path to a database, or a flake-reference."
        );
      }
  }


  /**
   * Add `target` argument to any parser to read either a `flake-ref` or
   * path to an existing database.
   */
    argparse::Argument &
  addTargetArg( argparse::ArgumentParser & parser )
  {
    return parser.add_argument( "target" )
                 .help( "The source ( database path or flake-ref ) to read" )
                 .required()
                 .metavar( "DB-OR-FLAKE-REF" )
                 .action(
                   [&]( const std::string & target )
                   {
                     if ( flox::isSQLiteDb( target ) )
                       {
                         this->dbPath = nix::absPath( target );
                       }
                     else  /* flake-ref */
                       {
                         this->parseFloxFlake( target );
                       }
                     this->openPkgDb();
                   }
                 );
  }

};  /* End struct `PkgDbMixin' */


/* -------------------------------------------------------------------------- */

struct AttrPathMixin {

  flox::AttrPath attrPath;

  /* Sets the attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable".
   */
    argparse::Argument &
  addAttrPathArgs( argparse::ArgumentParser & parser )
  {
    return parser.add_argument( "attr-path" )
                 .help( "Attribute path to scrape" )
                 .metavar( "ATTR" )
                 .remaining()
                 .default_value( std::vector<std::string> {
                   "packages"
                 , nix::settings.thisSystem.get()
                 } )
                 .action( [&]( const std::string & p )
                          {
                            this->attrPath.emplace_back( p );
                          }
                        );
  }

    void
  postProcessArgs()
  {
    if ( this->attrPath.size() < 2 )
      {
        this->attrPath.push_back(
          nix::settings.thisSystem.get()
        );
      }
    if ( ( this->attrPath.size() < 3 ) &&
         ( this->attrPath[0] == "catalog" )
       )
      {
        this->attrPath.push_back( "stable" );
      }
  }

};  /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

/** Adds optional `-f,--force` flag with associated variable to a state blob. */
struct ForceMixin {

  bool force = false;

    auto
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
    this->openPkgDb();
    didPost = true;
  }

    int
  run()
  {
    this->postProcessArgs();

    /* If we haven't processed this prefix before or `--force' was given, open
     * the eval cache and start scraping. */

    if ( this->force || ( ! this->db->hasPackageSet( this->attrPath ) ) )
      {
        std::vector<nix::Symbol> symbolPath;
        for ( const auto & a : this->attrPath )
          {
            symbolPath.emplace_back( this->state->symbols.create( a ) );
          }

        flox::pkgdb::Todos todo;
        if ( flox::MaybeCursor root =
               this->flake->maybeOpenCursor( symbolPath );
             root != nullptr
           )
          {
            todo.push(
              std::make_pair( this->attrPath, (flox::Cursor) root )
            );
          }

        while ( ! todo.empty() )
          {
            auto & [prefix, cursor] = todo.front();
            flox::pkgdb::scrape(
              * this->db
            , this->state->symbols
            , prefix
            , cursor
            , todo
            );
            todo.pop();
          }
      }

    /* Print path to database. */
    std::cout << this->dbPath.value() << std::endl;
    return EXIT_SUCCESS;  /* GG, GG */
  }

};  /* End struct `ScrapeEnv' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
