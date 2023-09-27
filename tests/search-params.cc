/* ========================================================================== *
 *
 * @file tests/search-params.cc
 *
 * @brief Minimal executable that parses a @a flox::search::SearchParams struct.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>
#include <algorithm>

#include <nlohmann/json.hpp>

#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  void
printInput( const auto & pair )
{
  const std::string                               & name   = pair.first;
  const flox::RegistryInput & params = pair.second;
  std::cout << "    " << name << std::endl
            << "      subtrees: "
            << nlohmann::json( params.subtrees ).dump() << std::endl
            << "      stabilities: "
            << nlohmann::json(
                 params.stabilities.value_or( std::vector<std::string> {} )
               ).dump() << std::endl;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{
  flox::search::SearchParams params;

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
          , "floxpkgs": {
              "from": {
                "type": "github"
              , "owner": "flox"
              , "repo": "floxpkgs"
              }
            , "subtrees": ["catalog"]
            , "stabilities": ["stable"]
            }
          }
        , "defaults": {
            "subtrees": null
          , "stabilities": ["stable"]
          }
        , "priority": ["nixpkgs", "floco", "floxpkgs"]
        }
      , "systems": ["x86_64-linux"]
      , "allow":   { "unfree": true, "broken": false, "licenses": ["MIT"] }
      , "semver":  { "preferPreReleases": false }
      , "query":   { "partialMatch": "hello" }
      } )" ).get_to( params );
    }
  else
    {
      nlohmann::json::parse( argv[1] ).get_to( params );
    }

  std::cout << nlohmann::json( params ).dump() << std::endl;


  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
