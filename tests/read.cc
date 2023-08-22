/* ========================================================================== *
 *
 * @file read.cc
 *
 * @brief Tests for `read.cc`.
 *
 *
 * -------------------------------------------------------------------------- */

#include <assert.h>

#include <nlohmann/json.hpp>
#include "flox/pkgdb/read.hh"
#include "test.hh"
#include "flox/raw-package.hh"

/* -------------------------------------------------------------------------- */

using namespace flox;

/* -------------------------------------------------------------------------- */

/**
 * Test ability to add `AttrSet` rows.
 * This test should run before all others since it essentially expects
 * `AttrSets` to be empty.
 */
  bool
test_distanceFromMatch()
{
  std::tuple<char const*, char const*, size_t> cases[] = {
    { "match", "match", 0 },
    { "match", "partial match", 0 },
    { "match", "miss", 0 },
    { "partial match", "match", 1 },
    { "partial match", "partial match", 1 },
    { "partial match", "miss", 2 },
    { "miss", "match", 3 },
    { "miss", "partial match", 3 },
    { "miss", "miss", 4 },
  };

  RawPackage pkg;
  for (auto [pname, description, distance] : cases) {
      pkg = RawPackage(nlohmann::json {
        { "name", "name" },
        { "pname", pname },
        { "description", description },
      });
      EXPECT_EQ(*pkgdb::distanceFromMatch(pkg, "match"), distance);
  }

  // Should return std::nullopt for empty match string.
  EXPECT(pkgdb::distanceFromMatch(pkg, "") == std::nullopt);
  return true;
}

/* ========================================================================== */

  int
main( int argc, char * argv[] )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  nix::Verbosity verbosity;
  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) )
    {
      verbosity = nix::lvlDebug;
    }
  else
    {
      verbosity = nix::lvlWarn;
    }

/* -------------------------------------------------------------------------- */

  {
    RUN_TEST( distanceFromMatch );
  }

/* -------------------------------------------------------------------------- */

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
