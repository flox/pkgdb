/* ========================================================================== *
 *
 * @file tests/resolver-params.cc
 *
 * @brief Minimal executable that parses a
 *        @a flox::resolver::ResolveOneParams struct.
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include <nlohmann/json.hpp>

#include "flox/resolver/resolve.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

void
printInput( const auto &pair )
{
  const std::string         &name   = pair.first;
  const flox::RegistryInput &params = pair.second;
  std::cout << "    " << name << std::endl
            << "      subtrees: " << nlohmann::json( params.subtrees ).dump()
            << std::endl;
}


/* -------------------------------------------------------------------------- */

int
main( int argc, char *argv[] )
{
  flox::resolver::ResolveOneParams params;

  if ( argc < 2 )
    {
      nlohmann::json::parse( R"( {
        "registry": {
          "inputs": {
            "nixpkgs": {
              "from": {
                "type": "github"
              , "owner": "NixOS"
              , "repo": "nixpkgs"
              , "rev":  "e8039594435c68eb4f780f3e9bf3972a7399c4b1"
              }
            , "subtrees": ["legacyPackages"]
            }
          , "floco": {
              "from": {
                "type": "github"
              , "owner": "aakropotkin"
              , "repo": "floco"
              }
            , "subtrees": ["packages"]
            }
          , "nixpkgs-flox": {
              "from": {
                "type": "github"
              , "owner": "flox"
              , "repo": "nixpkgs-flox"
              }
            , "subtrees": ["catalog"]
            }
          }
        , "priority": ["nixpkgs", "floco", "nixpkgs-flox"]
        }
      , "systems": ["x86_64-linux"]
      , "query":   {
          "pname": "hello"
        , "semver": ">=2"
        }
      } )" )
        .get_to( params );
    }
  else { nlohmann::json::parse( argv[1] ).get_to( params ); }

  auto state = flox::resolver::ResolverState(
    params.registry,
    dynamic_cast<const flox::pkgdb::QueryPreferences &>( params ) );

  auto descriptor = params.query;

  for ( const auto &resolved : flox::resolver::resolve_v0( state, descriptor ) )
    {
      std::cout << nlohmann::json( resolved ).dump() << std::endl;
    }


  std::cout << std::endl;

  auto input = flox::resolver::Resolved::Input( nlohmann::json {

  } );

  auto resolved = flox::resolver::Resolved {
    .input = flox::resolver::Resolved::Input(
      "nixpkgs",
      nlohmann::json {
        { "type", "github" },
        { "owner", "NixOS" },
        { "repo", "nixpkgs" },
        { "rev", "e8039594435c68eb4f780f3e9bf3972a7399c4b1" } } ),
    .path = flox::AttrPath { "legacyPackages", "x86_64-linux", "hello" },
    .info = nlohmann::json::object()
  };

  auto resolvedJSON = nlohmann::json( resolved );
  std::cout << resolvedJSON.dump() << std::endl;

  resolvedJSON.emplace( "phony", 1 );
  std::cout << resolvedJSON.dump() << std::endl;

  /* Adding junk fields does NOT throw an error, but they are stripped. */
  auto resolved2 = resolvedJSON.get<flox::resolver::Resolved>();
  std::cout << nlohmann::json( resolved2 ).dump() << std::endl;

  /* Assignment completely resets the object, stripping extra fields. */
  resolvedJSON = resolved2;
  std::cout << resolvedJSON.dump() << std::endl;

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
