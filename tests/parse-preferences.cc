/* ========================================================================== *
 *
 * @file tests/parse-preferences.cc
 *
 * @brief Minimal executable that parses a `flox::search::Preferences` struct.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>

#include <nlohmann/json.hpp>

#include "flox/search/preferences.hh"


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
  nlohmann::json jparams;
  if ( argc < 2 )
    {
      jparams = R"( {
        "registry": {
          "inputs": {
            "nixpkgs": {
              "from": {
                "type": "github"
              , "owner": "NixOS"
              , "repo": "nixpkgs"
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
      } )"_json;
    }
  else
    {
      jparams = nlohmann::json::parse( argv[1] );
    }

  flox::search::SearchParams params;

  flox::search::from_json( jparams, params );

  std::cout << "registry:" << std::endl << "  inputs:" << std::endl;
  for ( const auto & pair : params.registry.inputs ) { printInput( pair ); }
  std::cout << "  defaults:" << std::endl
            << "    subtrees: " << nlohmann::json(
                                     params.registry.defaults.subtrees
                                   ).dump() << std::endl
            << "    stabilities: " << nlohmann::json(
                                        params.registry.defaults.stabilities
                                      ).dump() << std::endl;

  std::cout << "systems: " << nlohmann::json( params.systems ).dump()
            << std::endl;

  /* Allow */
  std::cout << "allow:" << std::endl
            << "  unfree: " << nlohmann::json( params.allow.unfree ).dump()
            << std::endl
            << "  broken: " << nlohmann::json( params.allow.broken ).dump()
            << std::endl
            << "  licenses: "
            << nlohmann::json(
                 params.allow.licenses.value_or( std::vector<std::string> {} )
               ).dump() << std::endl;

  /* Semver */
  std::cout << "semver:" << std::endl
            << "  preferPreReleases: "
            << nlohmann::json( params.semver.preferPreReleases ).dump()
            << std::endl;

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
