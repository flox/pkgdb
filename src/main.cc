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
#include "flox/core/exceptions.hh"
#include "flox/pkgdb/command.hh"
#include "flox/pkgdb/write.hh"
#include "flox/resolver/command.hh"
#include "flox/search/command.hh"


/* -------------------------------------------------------------------------- */

int
run( int argc, char *argv[] )
{
  /* Define arg parsers. */

  flox::command::VerboseParser prog( "pkgdb", FLOX_PKGDB_VERSION );
  prog.add_description( "CRUD operations for package metadata" );

  flox::pkgdb::ScrapeCommand cmdScrape;
  prog.add_subparser( cmdScrape.getParser() );

  flox::pkgdb::GetCommand cmdGet;
  prog.add_subparser( cmdGet.getParser() );

  flox::pkgdb::ListCommand cmdList;
  prog.add_subparser( cmdList.getParser() );

  flox::search::SearchCommand cmdSearch;
  prog.add_subparser( cmdSearch.getParser() );

  flox::resolver::ResolveCommand cmdResolve;
  prog.add_subparser( cmdResolve.getParser() );

  flox::resolver::LockCommand cmdLock;
  prog.add_subparser( cmdLock.getParser() );


  /* Parse Args */

  try
    {
      prog.parse_args( argc, argv );
    }
  catch ( const std::runtime_error &err )
    {
      throw flox::command::InvalidArgException( err.what() );
    }

  /* Run subcommand */

  if ( prog.is_subcommand_used( "scrape" ) ) { return cmdScrape.run(); }
  if ( prog.is_subcommand_used( "get" ) ) { return cmdGet.run(); }
  if ( prog.is_subcommand_used( "list" ) ) { return cmdList.run(); }
  if ( prog.is_subcommand_used( "search" ) ) { return cmdSearch.run(); }
  if ( prog.is_subcommand_used( "resolve" ) ) { return cmdResolve.run(); }
  if ( prog.is_subcommand_used( "lock" ) ) { return cmdLock.run(); }

  // TODO: better error for this,
  // likely only occurs if we add a new command without handling it (?)
  throw flox::FloxException( "unrecognized command" );
}


/* -------------------------------------------------------------------------- */

int
main( int argc, char *argv[] )
{
  try
    {
      return run( argc, argv );
    }
  catch ( const flox::FloxException &err )
    {
      if ( ! isatty( STDERR_FILENO ) )
        {
          std::cout << nlohmann::json( err ).dump() << std::endl;
        }
      else { std::cerr << err.what() << std::endl; }

      return err.getErrorCode();
    }
  catch ( const std::exception &err )
    {
      if ( ! isatty( STDERR_FILENO ) )
        {
          nlohmann::json error = {
            { "exit_code", EXIT_FAILURE },
            { "message", err.what() },
          };
          std::cout << error << std::endl;
        }
      else { std::cerr << err.what() << std::endl; }

      return flox::EC_FAILURE;
    }
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
