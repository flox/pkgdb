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
#include "flox/pkgdb/write.hh"
#include "flox/pkgdb/command.hh"
#include "flox/search/command.hh"
#include "flox/resolver/command.hh"


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

  flox::pkgdb::ListCommand cmdList;
  prog.add_subparser( cmdList.parser );

  flox::search::SearchCommand cmdSearch;
  prog.add_subparser( cmdSearch.parser );

  flox::resolver::ResolveCommand cmdResolve;
  prog.add_subparser( cmdResolve.parser );


  /* Parse Args */

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const flox::pkgdb::PkgQuery::InvalidArgException & err )
    {
      // TODO: DRY ( see search/command.cc )
      int exitCode = flox::EC_PKG_QUERY_INVALID_ARG;
      exitCode += static_cast<int>( err.errorCode );
      for ( int i = 1; i < argc; ++i )
        {
          if ( std::string_view( argv[i] ) == "search" )
            {
              std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                        << exitCode << " }" << std::endl;
              return exitCode;
            }
        }
      std::cerr << "ERROR: " << err.what() << std::endl;
      std::cerr << prog << std::endl;
      return exitCode;
    }
  catch( const std::exception & err )
    {
      // TODO: DRY ( see search/command.cc )
      for ( int i = 1; i < argc; ++i )
        {
          if ( std::string_view( argv[i] ) == "search" )
            {
              std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                        << flox::EC_FAILURE << " }" << std::endl;
              return flox::EC_FAILURE;
            }
        }
      std::cerr << "ERROR: " << err.what() << std::endl;
      std::cerr << prog << std::endl;
      return flox::EC_FAILURE;
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
      if ( prog.is_subcommand_used( "list" ) )
        {
          return cmdList.run();
        }
      if ( prog.is_subcommand_used( "search" ) )
        {
          return cmdSearch.run();
        }
      if ( prog.is_subcommand_used( "resolve" ) )
        {
          return cmdResolve.run();
        }
    }
  catch( const flox::pkgdb::PkgQuery::InvalidArgException & err )
    {
      // TODO: DRY ( see search/command.cc )
      int exitCode = flox::EC_PKG_QUERY_INVALID_ARG;
      exitCode += static_cast<int>( err.errorCode );
      for ( int i = 1; i < argc; ++i )
        {
          if ( std::string_view( argv[i] ) == "search" )
            {
              std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                        << exitCode << " }" << std::endl;
              return exitCode;
            }
        }
      std::cerr << "ERROR: " << err.what() << std::endl;
      std::cerr << prog << std::endl;
      return exitCode;
    }
  catch( const std::exception & err )
    {
      // TODO: DRY ( see search/command.cc )
      for ( int i = 1; i < argc; ++i )
        {
          if ( std::string_view( argv[i] ) == "search" )
            {
              std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                        << flox::EC_FAILURE << " }" << std::endl;
              return flox::EC_FAILURE;
            }
        }
      std::cerr << "ERROR: " << err.what() << std::endl;
      std::cerr << prog << std::endl;
      return flox::EC_FAILURE;
    }

  return EXIT_FAILURE;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
