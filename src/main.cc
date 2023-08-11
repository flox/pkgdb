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

struct VerboseParser;


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


  private:



    PkgDbEnv( int argc, char * argv[] )
      : flox::NixState()
      , pPkgDb( "pkgdb", FLOX_PKGDB_VERSION )
      , pScrape( "scrape" )
    {
      this->pPkgDb.add_description( "CRUD operations for package metadata" );
      PkgDbEnv::addVerbosity( this->pPkgDb );

      this->pScrape.add_description( "Scrape a flake and emit a SQLite3 DB" );
      PkgDbEnv::addVerbosity( this->pScrape );

      this->pScrape.add_argument( "-f", "--force" )
        .help( "Force re-evaluation of prefix" )
        .default_value( false )
        .implicit_value( true )
        .nargs( 0 )
        .action( [&]( const bool & v ) { this->force = v; } )
      ;

      /* Add the scrape subcommand. */
      this->pPkgDb.add_subparser( this->pScrape );

    }
};


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{

  /* Define arg parsers. */

  #if 0
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
  #endif

  return EXIT_FAILURE;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
