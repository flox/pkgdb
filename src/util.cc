/* ========================================================================== *
 *
 * @file flox/util.cc
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdio>
#include <filesystem>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/core/util.hh"
#include "flox/core/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Perform one time `nix` runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
  void
initNix( nix::Verbosity verbosity )
{
  static bool didNixInit = false;
  if ( didNixInit ) { return; }

  /* Assign verbosity to `nix' global setting */
  nix::verbosity = verbosity;
  nix::setStackSize( ( ( (size_t) 64 ) * 1024 ) * 1024 );  // NOLINT
  nix::initNix();
  nix::initGC();
  /* Suppress benign warnings about `nix.conf'. */
  nix::verbosity = nix::lvlError;
  nix::initPlugins();
  /* Restore verbosity to `nix' global setting */
  nix::verbosity = verbosity;

  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.setDefault( true );

  didNixInit = true;
}


/* -------------------------------------------------------------------------- */

  bool
isSQLiteDb( const std::string & dbPath )
{
  std::filesystem::path path( dbPath );
  if ( ! std::filesystem::exists( path ) )     { return false; }
  if ( std::filesystem::is_directory( path ) ) { return false; }

  /* Read file magic */
  static const char expectedMagic[16] = "SQLite format 3";  // NOLINT

  char buffer[16];  // NOLINT
  std::memset( & buffer[0], '\0', sizeof( buffer ) );
  FILE * filep = fopen( dbPath.c_str(), "rb" );

  std::clearerr( filep );

  const size_t nread =
    std::fread( & buffer[0], sizeof( buffer[0] ), sizeof( buffer ), filep );
  if ( nread != sizeof( buffer ) )
    {
      if ( std::feof( filep ) != 0 )
        {
          std::fclose( filep );  // NOLINT
          return false;
        }
      if ( std::ferror( filep ) != 0 )
        {
          std::fclose( filep );  // NOLINT
          throw flox::FloxException( "Failed to read file " + dbPath );
        }
      std::fclose( filep );  // NOLINT
      return false;
    }
  std::fclose( filep );  // NOLINT
  return std::string_view( & buffer[0] ) ==
         std::string_view( & expectedMagic[0] );
}


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
