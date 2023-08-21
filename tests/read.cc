/* ========================================================================== *
 *
 * @file read.cc
 *
 * @brief Tests for `read.cc`.
 *
 *
 * -------------------------------------------------------------------------- */

#include <assert.h>

#include "flox/pkgdb/read.hh"
#include "test.hh"
#include "flox/flake-package.hh"

/* -------------------------------------------------------------------------- */

/**
 * Test ability to add `AttrSet` rows.
 * This test should run before all others since it essentially expects
 * `AttrSets` to be empty.
 */
  bool
test_distanceFromMatch()
{
  // auto pkg = flox::FlakePackage();
  // flox::pkgdb::distanceFromMatch(pkg, "match");
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
