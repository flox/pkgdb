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

#include "flox/util.hh"
#include "flox/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

static bool didNixInit = false;

/**
 * Perform one time `nix` runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
  void
initNix( nix::Verbosity verbosity )
{
  if ( didNixInit ) { return; }

  /* Assign verbosity to `nix' global setting */
  nix::verbosity = verbosity;
  nix::setStackSize( 64 * 1024 * 1024 );
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
  static const char expectedMagic[16] = "SQLite format 3";

  char buffer[16];
  std::memset( buffer, '\0', sizeof( buffer ) );
  FILE * fp = fopen( dbPath.c_str(), "rb" );

  std::clearerr( fp );

  const size_t nread =
    std::fread( buffer, sizeof( buffer[0] ), sizeof( buffer ), fp );
  if ( nread != sizeof( buffer ) )
    {
      if ( std::feof( fp ) )
        {
          std::fclose( fp );
          return false;
        }
      else if ( std::ferror( fp ) )
        {
          std::fclose( fp );
          throw flox::FloxException( "Failed to read file " + dbPath );
        }
      else
        {
          std::fclose( fp );
          return false;
        }
    }
  else
    {
      std::fclose( fp );
      return std::string_view( buffer ) == std::string_view( expectedMagic );
    }
}


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
