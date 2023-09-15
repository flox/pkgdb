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

#include <nlohmann/json.hpp>

#include "flox/registry.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/* Initialized in `main' */
static flox::RegistryRaw commonRegistry;  // NOLINT


/* -------------------------------------------------------------------------- */

  bool
test_FloxFlakeInputRegistry0()
{
  using namespace flox;

  FloxFlakeInputFactory           factory;
  Registry<FloxFlakeInputFactory> registry( commonRegistry, factory );
  size_t count = 0;
  for ( const auto & [name, flake] : registry )
    {
      (void) flake->getFlakeRef();
      ++count;
    }

  EXPECT_EQ( count, static_cast<size_t>( 3 ) );

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

  /* Initialize common registry. */
  flox::from_json( R"( {
      "inputs": {
        "nixpkgs": {
          "from": {
            "type": "github"
          , "owner": "NixOS"
          , "repo": "nixpkgs"
          , "rev": "e8039594435c68eb4f780f3e9bf3972a7399c4b1"
          }
        , "subtrees": ["legacyPackages"]
        }
      , "floco": {
          "from": {
            "type": "github"
          , "owner": "aakropotkin"
          , "repo": "floco"
          , "rev": "1e84b4b16bba5746e1195fa3a4d8addaaf2d9ef4"
          }
        , "subtrees": ["packages"]
        }
      , "nixpkgs-flox": {
          "from": {
            "type": "github"
          , "owner": "flox"
          , "repo": "nixpkgs-flox"
          , "rev": "feb593b6844a96dd4e17497edaabac009be05709"
          }
        , "subtrees": ["catalog"]
        , "stabilities": ["stable"]
        }
      }
    , "defaults": {
        "subtrees": null
      , "stabilities": ["stable"]
      }
    , "priority": ["nixpkgs", "floco", "nixpkgs-flox"]
    } )"_json
  , commonRegistry
  );


/* -------------------------------------------------------------------------- */

  {

    RUN_TEST( FloxFlakeInputRegistry0 );

  }


  return exitCode;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
