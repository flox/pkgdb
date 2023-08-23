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
    { "match", "match",                 flox::pkgdb::MS_EXACT_PNAME        }
  , { "match", "partial match",         flox::pkgdb::MS_EXACT_PNAME        }
  , { "match", "miss",                  flox::pkgdb::MS_EXACT_PNAME        }
  , { "partial match", "match",         flox::pkgdb::MS_PARTIAL_PNAME_DESC }
  , { "partial match", "partial match", flox::pkgdb::MS_PARTIAL_PNAME_DESC }
  , { "partial match", "miss",          flox::pkgdb::MS_PARTIAL_PNAME      }
  , { "miss", "match",                  flox::pkgdb::MS_PARTIAL_DESC       }
  , { "miss", "partial match",          flox::pkgdb::MS_PARTIAL_DESC       }
  , { "miss", "miss",                   flox::pkgdb::MS_NONE               }
  };

  RawPackage pkg;
  for ( auto [pname, description, distance] : cases )
    {
      pkg = RawPackage( nlohmann::json {
        { "name", "name" }
      , { "pname", pname }
      , { "description", description }
      } );
      EXPECT_EQ( pkgdb::distanceFromMatch( pkg, "match"), distance );
    }

  /* Should return std::nullopt for empty match string. */
  EXPECT( pkgdb::distanceFromMatch( pkg, "" ) == flox::pkgdb::MS_NONE );
  return true;
}

/* ========================================================================== */

  int
main( int argc, char * argv[] )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) )
    {
      nix::verbosity = nix::lvlDebug;
    }
  else
    {
      nix::verbosity = nix::lvlWarn;
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
