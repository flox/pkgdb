/* ========================================================================== *
 *
 * @file main.cc
 *
 * @brief Executable exposing CRUD operations for package metadata.
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

#include "flox/util.hh"
#include "flox/flox-flake.hh"
#include "pkgdb.hh"


/* -------------------------------------------------------------------------- */

struct PkgDbEnv : public flox::NixState {
  public:

    argparse::ArgumentParser pPkgDb;
    argparse::ArgumentParser pScrape;

    struct GetParsers : argparse::ArgumentParser {
      argparse::ArgumentParser path;
      argparse::ArgumentParser lockedFlake;
      argparse::ArgumentParser attrId;
      argparse::ArgumentParser pkgId;
    } pGet;

    std::string subcommand;

    std::optional<flox::FloxFlake>      flake;
    std::string                         dbPath;
    std::unique_ptr<flox::pkgdb::PkgDb> db;
    std::optional<flox::AttrPath>       attrPath;
    std::optional<bool>                 force;


  private:

      void
    parseFlakeRefArg( const std::string & arg )
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
        this->flake = std::make_optional(
          flox::FloxFlake( (nix::ref<nix::EvalState>) this->state, ref )
        );
      }

      if ( ! this->flake.value().lockedFlake
                        .flake.lockedRef.input.hasAllInfo()
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


  public:

    /** Add verbosity flags to any parser and modify the global verbosity. */
      static void
    addVerbosity( argparse::ArgumentParser & parser )
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
      parser.add_argument( "-q", "--quiet" )
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
        .append()
      ;
      parser.add_argument( "-v", "--verbose" )
        .help( "Increase the logging verbosity level. May be up to 4 times." )
        .action(
          [&]( const auto & )
          {
            nix::verbosity =
              ( nix::lvlVomit <= nix::verbosity )
                ? nix::lvlVomit : (nix::Verbosity) ( nix::verbosity + 1 );
          }
        ).default_value( false ).implicit_value( true )
        .append()
      ;
    }


    /**
     * Add `target` argument to any parser to read either a `flake-ref` or
     * path to an existing database.
     */
      void
    addTarget( argparse::ArgumentParser & parser )
    {
      parser.add_argument( "target" )
        .help(
          "The source (database path or flake-ref) to read"
        )
        .required()
        .metavar( "DB-OR-FLAKE-REF" )
        .action(
          [&]( const std::string & target )
          {
            if ( isSQLiteDb( target ) )
              {
                this->flake  = std::nullopt;
                this->dbPath = nix::absPath( target );
                this->db     =
                  std::make_unique<flox::pkgdb::PkgDb>( this->dbPath );
              }
            else  /* flake-ref */
              {
                this->parseFlakeRefArg( target );

                this->dbPath =
                  flox::pkgdb::genPkgDbName( this->flake.value().lockedFlake );

                this->db = std::make_unique<flox::pkgdb::PkgDb>(
                  this->flake.value().lockedFlake, this->dbPath
                );
              }
          }
        )
      ;
    }


    PkgDbEnv( int argc, char * argv[] )
      : flox::NixState()
      , pPkgDb( "pkgdb", FLOX_PKGDB_VERSION )
      , pScrape( "scrape" )
    {
      this->pPkgDb.add_description( "CRUD operations for package metadata" );
      PkgDbEnv::addVerbosity( this->pPkgDb );

      this->pScrape.add_description( "Scrape a flake and emit a SQLite3 DB" );
      PkgDbEnv::addVerbosity( this->pScrape );

      this->pScrape.add_argument( "-d", "--database" )
        .help( "Use database at PATH" )
        .default_value( "" )
        .metavar( "PATH" )
        .nargs( 1 )
        .action( [&]( const std::string & dbPath )
                 {
                   this->dbPath = nix::absPath( dbPath );
                 }
               )
      ;

      this->pScrape.add_argument( "-f", "--force" )
        .help( "Force re-evaluation of prefix" )
        .default_value( false )
        .implicit_value( true )
        .nargs( 0 )
        .action( [&]( const bool & v ) { this->force = v; } )
      ;

      this->pScrape.add_argument( "flake-ref" )
        .help( "A flake-reference URI string ( preferably locked )" )
        .required()
        .metavar( "FLAKE-REF" )
        .action( [&]( const std::string & target )
                 {
                   this->parseFlakeRefArg( target );
                 }
               )
      ;

      /* Sets the attribute path to be scraped.
       * If no system is given use the current system.
       * If we're searching a catalog and no stability is given, use "stable".
       */
      this->pScrape.add_argument( "attr-path" )
        .help( "Attribute path to scrape" )
        .metavar( "ATTR" )
        .remaining()
        .default_value( flox::AttrPath {
          "packages"
        , nix::settings.thisSystem.get()
        } )
        .action( [&]( const flox::AttrPath & given )
                 {
                   flox::AttrPath path = given;
                   if ( path.size() < 2 )
                     {
                       path.push_back( nix::settings.thisSystem.get() );
                     }
                   if ( ( path.size() < 3 ) && ( path[0] == "catalog" ) )
                     {
                       path.push_back( "stable" );
                     }
                   this->attrPath = std::make_optional( path );
                 }
               )
      ;

      /* Add the scrape subcommand. */
      this->pPkgDb.add_subparser( this->pScrape );

    }
};


/* -------------------------------------------------------------------------- */

  static int
scrapeImplementation( argparse::ArgumentParser & cmdScrape
                    , nix::Verbosity           & verbosity
                    )
{

  /* Initialize `nix' */

  // TODO: Handle `--store'
  flox::NixState nstate( verbosity );


  /* Fetch and lock our flake. */


  /* Open our database. */

  /* Maybe use path from arguments. */
  std::string dbPathStr;
  if ( cmdScrape.is_used( "-d" ) )
    {
      dbPathStr = cmdScrape.get<std::string>( "-d" );
      if ( ! dbPathStr.empty() ) { dbPathStr = nix::absPath( dbPathStr ); }
    }

  /* Fallback to generated path under `XDG_CACHE_DIR/flox/pkgdb-v0/' */
  if ( dbPathStr.empty() )
    {
      dbPathStr = flox::pkgdb::genPkgDbName( flake.lockedFlake );
    }

  /* Create directories if needed. */
  std::filesystem::path dbPath( dbPathStr );
  if ( ! std::filesystem::exists( dbPath.parent_path() ) )
    {
      std::filesystem::create_directories( dbPath.parent_path() );
    }

  /* Open/Create database. */
  flox::pkgdb::PkgDb db( flake.lockedFlake, dbPathStr );


  /* If we haven't processed this prefix before or `--force' was given, open the
   * eval cache and start scraping. */

  if ( cmdScrape.get<bool>( "-f" ) || ( ! db.hasPackageSet( attrPath ) ) )
    {
      std::vector<nix::Symbol> symbolPath;
      for ( const auto & a : attrPath )
        {
          symbolPath.emplace_back( flake.state->symbols.create( a ) );
        }

      flox::pkgdb::Todos todo;
      if ( flox::MaybeCursor root = flake.maybeOpenCursor( symbolPath );
           root != nullptr
         )
        {
          todo.push(
            std::make_pair( std::move( attrPath ), (flox::Cursor) root )
          );
        }

      while ( ! todo.empty() )
        {
          auto & [prefix, cursor] = todo.front();
          scrape( db, flake.state->symbols, prefix, cursor, todo );
          todo.pop();
        }
    }


  /* Print path to database. */
  std::cout << dbPathStr << std::endl;

  return EXIT_SUCCESS;  /* GG, GG */
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{

  /* Define arg parsers. */

  argparse::ArgumentParser cmdGet( "get" );
  cmdGet.add_description( "Query package attributes" );

  addVerbosity( cmdGet );

  argparse::ArgumentParser cmdGetPath( "path", "p" );
  cmdGetPath.add_description(
    "Print the attribute path associated with an AttrSets.id or Packages.id"
  );
  addTarget( cmdGetPath );
  cmdGetPath.add_argument( "id" )
    .help( "The AttrSets.id or Packages.id to look up" )
    .required()
    .metavar( "ID" )
  ;
  cmdGetPath.add_argument( "-p", "--pkg" )
    .help(
      "Indicate that the id refers to a Packages.id rather than an AttrSets.id"
    )
    .default_value( false )
    .implicit_value( true )
    .nargs( 0 )
  ;
  cmdGet.add_subparser( cmdGetPath );


  prog.add_subparser( cmdGet );
  

/* -------------------------------------------------------------------------- */

  /* Parse Args */

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const std::runtime_error & err )
    {
      std::cerr << err.what() << std::endl;
      std::cerr << prog;
      return EXIT_FAILURE;
    }

  if ( prog.is_subcommand_used( "scrape" ) )
    {
      return scrapeImplementation( cmdScrape, verbosity );
    }

  return EXIT_FAILURE;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
