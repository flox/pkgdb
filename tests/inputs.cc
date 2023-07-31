/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_InputsFromJSON1()
{
  nlohmann::json inputs = R"(
    {
      "nixpkgs":  "github:NixOS/nixpkgs"
    , "nixpkgs2": {
        "type": "github"
      , "owner": "NixOS"
      , "repo":  "nixpkgs"
      }
    }
  )"_json;
  Inputs d( inputs );
  return true;
}

  bool
test_InputsToJSON1()
{
  nlohmann::json inputs = R"(
    {
      "nixpkgs":  "github:NixOS/nixpkgs"
    , "nixpkgs2": {
        "type": "github"
      , "owner": "NixOS"
      , "repo":  "nixpkgs"
      }
    }
  )"_json;
  Inputs d( inputs );
  nlohmann::json j = d.toJSON();
  return j["nixpkgs"] == j["nixpkgs2"];
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )
  RUN_TEST( InputsFromJSON1 );
  RUN_TEST( InputsToJSON1 );
  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
