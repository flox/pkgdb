/* ========================================================================== *
 *
 * @file resolver.cc
 *
 * @brief Tests for `flox::resolver` interfaces.
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>

#include <nlohmann/json.hpp>

#include "test.hh"
#include "flox/resolver/resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/* Initialized in `main' */
static flox::RegistryRaw             commonRegistry;     // NOLINT
static flox::pkgdb::QueryPreferences commonPreferences;  // NOLINT


/* -------------------------------------------------------------------------- */

/** @brief Test basic resolution for `hello`. */
  bool
test_resolve0( flox::resolver::ResolverState & state )
{
  flox::resolver::Descriptor descriptor;
  descriptor.pname = "hello";

  auto rsl = flox::resolver::resolve_v0( state, descriptor );

  EXPECT_EQ( rsl.size(), static_cast<std::size_t>( 5 ) );

  return true;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{
  int exitCode = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( exitCode, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  nix::Verbosity verbosity = nix::lvlWarn;
  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) ) // NOLINT
    {
      verbosity = nix::lvlDebug;
    }

  /* Initialize `nix' */
  flox::NixState nstate( verbosity );


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
          , "rev": "2afd962bbd6745d4d101c2924de34c5326042928"
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

  /* Initialize common preferences. */
  flox::pkgdb::from_json( R"( {
      "systems": ["x86_64-linux"]
    , "allow": {
        "unfree": true
      , "broken": false
      , "licenses": null
      }
    , "semver": {
        "preferPreReleases": false
      }
    } )"_json
  , commonPreferences
  );


/* -------------------------------------------------------------------------- */

  /* Scrape common registry members. */
  auto state = flox::resolver::ResolverState( commonRegistry
                                            , commonPreferences
                                            );


/* -------------------------------------------------------------------------- */

  {

    RUN_TEST( resolve0, state );

  }


  return exitCode;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
