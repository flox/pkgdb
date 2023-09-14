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
static const nlohmann::json commonRegistry = R"( {
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
} )"_json;

static const nlohmann::json commonPreferences = R"( {
  "systems": ["x86_64-linux"]
, "allow": {
    "unfree": true
  , "broken": false
  , "licenses": null
  }
, "semver": {
    "preferPreReleases": false
  }
} )"_json;


/* -------------------------------------------------------------------------- */

/** @brief Test basic resolution for `hello`. */
  bool
test_resolve0()
{
  flox::RegistryRaw             registry    = commonRegistry;
  flox::pkgdb::QueryPreferences preferences = commonPreferences;

  auto state = flox::resolver::ResolverState( registry, preferences );

  flox::resolver::Descriptor descriptor;
  descriptor.pname = "hello";

  auto rsl = flox::resolver::resolve_v0( state, descriptor );

  EXPECT_EQ( rsl.size(), static_cast<std::size_t>( 5 ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Expand number of stabilites and ensure `nixpkgs` is still scanned. */
  bool
test_resolve1()
{
  nlohmann::json registryJSON = commonRegistry;
  registryJSON["inputs"]["nixpkgs-flox"]["stabilities"] = {
    "stable", "staging", "unstable"
  };
  flox::RegistryRaw registry = registryJSON;

  flox::pkgdb::QueryPreferences preferences = commonPreferences;

  auto state = flox::resolver::ResolverState( registry, preferences );

  flox::resolver::Descriptor descriptor;
  descriptor.pname = "hello";

  auto rsl = flox::resolver::resolve_v0( state, descriptor );

  EXPECT_EQ( rsl.size(), static_cast<std::size_t>( 13 ) );

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

  /* Make a temporary directory for cache DBs to ensure tests
   * are reproducible. */
  std::string cacheDir = nix::createTempDir();
  setenv( "PKGDB_CACHEDIR", cacheDir.c_str(), 1 );


/* -------------------------------------------------------------------------- */

  {

    RUN_TEST( resolve0 );
    RUN_TEST( resolve1 );

  }

  /* Cleanup the temporary directory. */
  nix::deletePath( cacheDir );

  return exitCode;

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
