/* ========================================================================== *
 *
 * @file tests/resolver-params.cc
 *
 * @brief Minimal executable that parses a
 *        @a flox::resolver::ResolveOneParams struct.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>
#include <algorithm>

#include <nlohmann/json.hpp>

#include "flox/resolver/params.hh"
#include "flox/resolver/state.hh"


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
            , "stabilities": ["stable", "staging", "unstable"]
            }
          }
        , "priority": ["nixpkgs", "floco", "nixpkgs-flox"]
        }
      , "systems": ["x86_64-linux"]
      , "query":   {
          "pname": "hello"
        , "semver": ">=2"
        }
      } )" ).get_to( params );
    }
  else
    {
      nlohmann::json::parse( argv[1] ).get_to( params );
    }

  //std::cout << nlohmann::json( params ).dump() << std::endl;

  flox::resolver::ResolverState state( params );

  flox::pkgdb::PkgQueryArgs args;
  for ( const auto & [name, input] : * state.getPkgDbRegistry() )
    {
      /* We will get `false` if we should skip this input entirely. */
      bool shouldSearch = params.fillPkgQueryArgs( name, args );
      if ( ! shouldSearch ) { continue; }
      auto query = flox::pkgdb::PkgQuery( args );
      auto dbRO  = input->getDbReadOnly();
      for ( const auto & row : query.execute( dbRO->db ) )
        {
          std::cout << input->getRowJSON( row ).dump() << std::endl;
        }
    }

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
