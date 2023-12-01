/* ========================================================================== *
 *
 * @file pkgdb/list.cc
 *
 * @brief Implementation of `pkgdb list` subcommand.
 *
 * Used to list a summary of all known `pkgdb` databases.
 *
 *
 * -------------------------------------------------------------------------- */

#include <ctime>
#include <iostream>
#include <sys/stat.h>


#include "flox/pkgdb/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

GCCommand::GCCommand() : parser( "gc" )
{
  this->parser.add_description( "Delete stale Package DBs" );

  this->parser.add_argument( "-c", "--cachedir" )
    .help( "delete databases in a given directory" )
    .metavar( "PATH" )
    .nargs( 1 )
    .default_value( getPkgDbCachedir() )
    .action( [&]( const std::string & cacheDir )
             { this->cacheDir = nix::absPath( cacheDir ); } );

  this->parser.add_argument( "-a", "--min-age" )
    .help( "minimum age in days" )
    .metavar( "AGE" )
    .nargs( 1 )
    .action( [&]( const std::string & minAgeStr )
             { this->gcStaleAgeDays = stoi( minAgeStr ); } );

  this->parser.add_argument( "--dry-run" )
    .help( "list which databases are deleted, but don't actually delete them" )
    .default_value( false )
    .implicit_value( true )
    .action( [&]( const auto & ) { this->dryRun = true; } );
}


/* -------------------------------------------------------------------------- */
int
GCCommand::run()
{
  std::filesystem::path cacheDir
    = this->cacheDir.value_or( getPkgDbCachedir() );

  /* Make sure the cache directory exists. */
  if ( ! std::filesystem::exists( cacheDir ) )
    {
      /* If the user explicitly gave a directory, throw an error. */
      if ( this->cacheDir.has_value() )
        {
          std::cerr << "No such cachedir: " << cacheDir << std::endl;
          return EXIT_FAILURE;
        }
      /* Otherwise "they just don't have any databases", so don't error out." */
      return EXIT_SUCCESS;
    }

  std::vector<std::filesystem::path> to_delete;

  for ( const auto & entry : std::filesystem::directory_iterator( cacheDir ) )
    {
      if ( ! isSQLiteDb( entry.path() ) ) { continue; }

      struct stat result;

      if ( stat( entry.path().c_str(), &result ) == 0 )
        {
          auto access_time = std::chrono::system_clock::from_time_t(
            time( &result.st_atime ) );
          auto now = std::chrono::system_clock::now();

          auto age_in_days
            = std::chrono::duration_cast<std::chrono::days>( now - access_time )
                .count();

          if ( age_in_days >= this->gcStaleAgeDays )
            {
              to_delete.push_back( entry.path() );
            }
        }
      else
        {
          // ignore?
        }
    }

  fprintf( stderr, "Found %lu stale databases.\n", to_delete.size() );
  for ( const auto & path : to_delete )
    {
      std::cout << "deleting " << path;
      if ( this->dryRun ) { std::cout << " (dry run)" << std::endl; }
      else
        {
          std::cout << std::endl;
          std::filesystem::remove( path );
        }
    }

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
