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

#include "flox/command.hh"
#include "flox/util.hh"
#include "flox/flox-flake.hh"
#include "pkgdb.hh"


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{

  /* Define arg parsers. */

  flox::command::VerboseParser prog( "pkgdb", FLOX_PKGDB_VERSION );
  prog.add_description( "CRUD operations for package metadata" );

  flox::command::ScrapeCommand cmdScrape;
  prog.add_subparser( cmdScrape.parser );

  flox::command::GetCommand cmdGet;
  prog.add_subparser( cmdGet.parser );


  /* Parse Args */

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const std::runtime_error & err )
    {
      std::cerr << err.what() << std::endl;
      std::cerr << prog << std::endl;
      return EXIT_FAILURE;
    }


  /* Run subcommand */

  try
    {
      if ( prog.is_subcommand_used( "scrape" ) )
        {
          return cmdScrape.run();
        }
      if ( prog.is_subcommand_used( "get" ) )
        {
          return cmdGet.run();
        }
    }
  catch( const std::exception & err )
    {
      std::cerr << "ERROR: " << err.what() << std::endl;
    }

  return EXIT_FAILURE;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
