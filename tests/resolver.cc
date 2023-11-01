/* ========================================================================== *
 *
 * @file resolver.cc
 *
 * @brief Tests for `flox::resolver` interfaces.
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdlib>
#include <iostream>

#include <nlohmann/json.hpp>

#include "flox/resolver/resolve.hh"
#include "test.hh"


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
  }
  , "defaults": {
    "subtrees": null
  }
, "priority": ["nixpkgs", "floco"]
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

static const nlohmann::json resolvedRaw = R"( {
  "input": {
    "locked": {
      "owner": "NixOS",
      "repo": "nixpkgs",
      "rev": "e8039594435c68eb4f780f3e9bf3972a7399c4b1",
      "type": "github"
    },
    "name": "nixpkgs"
  },
  "path": [
    "legacyPackages",
    "x86_64-linux",
    "hello"
  ],
  "info": {
    "broken": false,
    "description": "A program that produces a familiar, friendly greeting",
    "id": 6095,
    "license": "GPL-3.0-or-later",
    "pkgSubPath": [
      "hello"
    ],
    "pname": "hello",
    "subtree": "legacyPackages",
    "system": "x86_64-linux",
    "unfree": false,
    "version": "2.12.1"
  }
} )"_json;

/* -------------------------------------------------------------------------- */

/** @brief Test `flox::resolver::Resolved` gets deserialized correctly. */
bool
test_deserialize_resolved()
{
  flox::resolver::Resolved resolved = resolvedRaw.template get<flox::resolver::Resolved>();

  // Do a non-exhaustive sanity check for now
  EXPECT_EQ( resolved.input.locked["owner"], "NixOS" );
  EXPECT_EQ( resolved.path[0], "legacyPackages" );
  EXPECT_EQ( resolved.info["broken"], false );

  return true;
}

/* -------------------------------------------------------------------------- */

/** @brief Test `flox::resolver::Resolved` gets serialized correctly. */
bool
test_serialize_resolved()
{
  flox::resolver::Resolved resolved = resolvedRaw.template get<flox::resolver::Resolved>();

  EXPECT_EQ( nlohmann::json( resolved ).dump(), resolvedRaw.dump() );

  return true;
}

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

  EXPECT_EQ( rsl.size(), std::size_t( 1 ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Limit resolution to a single input. */
bool
test_resolveInput0()
{
  flox::RegistryRaw             registry    = commonRegistry;
  flox::pkgdb::QueryPreferences preferences = commonPreferences;

  auto state = flox::resolver::ResolverState( registry, preferences );

  flox::resolver::Descriptor descriptor;
  descriptor.pname = "hello";
  descriptor.input = "nixpkgs";

  auto rsl = flox::resolver::resolve_v0( state, descriptor );

  EXPECT_EQ( rsl.size(), std::size_t( 1 ) );
  EXPECT( rsl.front().input.name.has_value() );
  EXPECT_EQ( *rsl.front().input.name, "nixpkgs" );

  return true;
}


/* -------------------------------------------------------------------------- */

int
main( int argc, char * argv[] )
{
  int exitCode = EXIT_SUCCESS;
#define RUN_TEST( ... ) _RUN_TEST( exitCode, __VA_ARGS__ )

  nix::verbosity = nix::lvlWarn;
  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) )  // NOLINT
    {
      nix::verbosity = nix::lvlDebug;
    }

  /* Make a temporary directory for cache DBs to ensure tests
   * are reproducible. */
  std::string cacheDir = nix::createTempDir();
  setenv( "PKGDB_CACHEDIR", cacheDir.c_str(), 1 );

  RUN_TEST( deserialize_resolved );
  RUN_TEST( serialize_resolved );
  RUN_TEST( resolve0 );
  RUN_TEST( resolveInput0 );

  /* Cleanup the temporary directory. */
  nix::deletePath( cacheDir );

  return exitCode;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
