/* ========================================================================== *
 *
 *  FIXME: real hashes
 *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "flox/resolver/lockfile.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

bool
test_LockedInputRawFromJSON0()
{
  using namespace flox::resolver;
  LockedInputRaw raw = R"(
    {
      "fingerprint": "xxxxxx",
      "url": "github:flox/flox/xxxxxx",
      "attrs": {
        "owner": "flox",
        "repo": "flox",
        "rev": "xxxxx"
      }
    }
  )"_json;
  return true;
}


/* -------------------------------------------------------------------------- */

bool
test_LockedPackageRawFromJSON0()
{
  using namespace flox::resolver;
  LockedPackageRaw raw = R"(
    {
      "input": {
        "fingerprint": "xxxxxx",
        "url": "github:flox/flox/xxxxxx",
        "attrs": {
          "owner": "flox",
          "repo": "flox",
          "rev": "xxxxx"
        }
      },
      "attr-path": ["legacyPackages", "x86_64-linux", "hello"],
      "priority": 5,
      "info": {}
    }
  )"_json;
  return true;
}


/* -------------------------------------------------------------------------- */

int
main()
{
  int exitCode = EXIT_SUCCESS;
  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RUN_TEST( ... ) _RUN_TEST( exitCode, __VA_ARGS__ )

  // RUN_TEST( LockedInputRawFromJSON0 );

  // RUN_TEST( LockedPackageRawFromJSON0 );

  return exitCode;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
