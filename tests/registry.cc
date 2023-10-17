/* ========================================================================== *
 *
 * @file registry.cc
 *
 * @brief Tests for `flox::Registry` interfaces.
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>
#include <fstream>

#include <nlohmann/json.hpp>

#include "flox/registry.hh"
#include "flox/core/command.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/* Initialized in `main' */
// static flox::RegistryRaw commonRegistry;  // NOLINT


/* -------------------------------------------------------------------------- */

  bool
test_FloxFlakeInputRegistry0()
{
  using namespace flox;

  std::ifstream regFile( TEST_DATA_DIR "/registry/registry0.json" );
  nlohmann::json json = nlohmann::json::parse( regFile );
  flox::RegistryRaw regRaw;
  json.get_to( regRaw );

  FloxFlakeInputFactory           factory;
  Registry<FloxFlakeInputFactory> registry( regRaw, factory );
  size_t count = 0;
  for ( const auto & [name, flake] : registry )
    {
      (void) flake->getFlakeRef();
      ++count;
    }

  EXPECT_EQ( count, std::size_t( 3 ) );

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_RegistryFileMixinHappyPath()
{
  using namespace flox;

  flox::command::RegistryFileMixin rfm;
  rfm.setRegistryPath( TEST_DATA_DIR "/registry/registry0.json" );
  RegistryRaw regRaw = rfm.getRegistryRaw();

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_RegistryFileMixinGetRegWithoutFile()
{
  using namespace flox;

  flox::command::RegistryFileMixin rfm;
  // rfm.setRegistryPath( TEST_DATA_DIR "/registry/registry0.json" );
  try
    {
      RegistryRaw regRaw = rfm.getRegistryRaw();
      return false;
    }
  catch ( FloxException & )
    {
      return true;
    }
}


/* -------------------------------------------------------------------------- */

  bool
test_RegistryFileMixinGetRegCached()
{
  using namespace flox;

  flox::command::RegistryFileMixin rfm;
  rfm.setRegistryPath( TEST_DATA_DIR "/registry/registry0.json" );
  RegistryRaw regRaw = rfm.getRegistryRaw();
  // You don't need the registry path if the registry is cached. If it's not
  // cached then you'll get an exception trying to open this file.
  rfm.registryPath = std::nullopt;
  RegistryRaw regRawCached = rfm.getRegistryRaw();

  return true;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{
  int exitCode = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( exitCode, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  nix::verbosity = nix::lvlWarn;
  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) ) // NOLINT
    {
      nix::verbosity = nix::lvlDebug;
    }

  /* Initialize `nix' */
  flox::NixState nstate;


/* -------------------------------------------------------------------------- */



/* -------------------------------------------------------------------------- */

  {

    RUN_TEST( FloxFlakeInputRegistry0 );
    RUN_TEST( RegistryFileMixinHappyPath );
    RUN_TEST( RegistryFileMixinGetRegWithoutFile );
    RUN_TEST( RegistryFileMixinGetRegCached );

  }


  return exitCode;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
