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
  const std::string                                 & name  = pair.first;
  const flox::search::Preferences::InputPreferences & prefs = pair.second;
  std::cout << "  " << name << std::endl
            << "    subtrees: "
            << nlohmann::json( prefs.subtrees ).dump() << std::endl
            << "    stabilities: "
            << nlohmann::json(
                 prefs.stabilities.value_or( std::vector<std::string> {} )
               ).dump() << std::endl;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{
  nlohmann::json jprefs;
  if ( argc < 2 )
    {
      jprefs = R"( {
        "inputs": [
          { "nixpkgs":  { "subtrees": ["legacyPackages"] } }
        , { "floco":    { "subtrees": ["packages"] } }
        , { "floxpkgs": { "subtrees": ["catalog"], "stabilities": ["stable"] } }
        ]
      , "systems": ["x86_64-linux"]
      , "allow":  { "unfree": true, "broken": false }
      , "semver": { "preferPreReleases": false }
      } )"_json;
    }
  else
    {
      jprefs = nlohmann::json::parse( argv[1] );
    }

  flox::search::Preferences prefs;

  flox::search::from_json( jprefs, prefs );

  std::cout << "inputs:" << std::endl;
  for ( const auto & pair : prefs.inputs ) { printInput( pair ); }

  std::cout << "systems: " << nlohmann::json( prefs.systems ).dump()
            << std::endl;

  /* Allow */
  std::cout << "allow:" << std::endl
            << "  unfree: " << nlohmann::json( prefs.allow.unfree ).dump()
            << std::endl
            << "  broken: " << nlohmann::json( prefs.allow.broken ).dump()
            << std::endl
            << "  licenses: "
            << nlohmann::json(
                 prefs.allow.licenses.value_or( std::vector<std::string> {} )
               ).dump() << std::endl;

  /* Semver */
  std::cout << "semver:" << std::endl
            << "  preferPreReleases: "
            << nlohmann::json( prefs.semver.preferPreReleases ).dump()
            << std::endl;

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
