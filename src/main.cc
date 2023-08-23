/* ========================================================================== *
 *
 * @file main.cc
 *
 * @brief Executable exposing CRUD operations for package metadata.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdlib>
#include <iostream>

#include "flox/core/command.hh"
#include "flox/pkgdb.hh"
#include "flox/pkgdb/command.hh"


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{

  /* Define arg parsers. */

  flox::command::VerboseParser prog( "pkgdb", FLOX_PKGDB_VERSION );
  prog.add_description( "CRUD operations for package metadata" );

  flox::pkgdb::ScrapeCommand cmdScrape;
  prog.add_subparser( cmdScrape.parser );

  flox::pkgdb::GetCommand cmdGet;
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
