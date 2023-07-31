/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <iostream>
#include <cstddef>


/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";

/**
 * These counts indicate the total number of derivations under
 * `<nixpkgsRef>#legacyPackages.x86_64-linux.**' which we will use to sanity
 * check calls to `size()'.
 * Note that the legacy implementation used to populate `DbPackageSet' will
 * fail to evaluate 3 packages which require `NIXPKGS_ALLOW_BROKEN', causing
 * different sizes to be collected ( until migration is coompleted ).
 */
static const size_t unbrokenPkgCount = 64037;
static const size_t fullPkgCount     = 64040;


/* -------------------------------------------------------------------------- */

static const std::string flocoPkgsRef =
  "github:aakropotkin/flocoPackages/2afd962bbd6745d4d101c2924de34c5326042928";


/* -------------------------------------------------------------------------- */

/** Wrap a test function pretty printing its name on failure. */
template <typename F, typename ... Args>
  static int
runTest( std::string_view name, F f, Args && ... args )
{
  try
    {
      if ( ! f( std::forward<Args>( args )... ) )
        {
          std::cerr << "  fail: " << name << std::endl;
          return EXIT_FAILURE;
        }
    }
  catch( const std::exception & e )
    {
      std::cerr << "  ERROR: " << name << ": " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

/**
 * Wrap a test routine which returns an exit code, and set a provided variable
 * to the resulting code on failure.
 * This pattern allows early tests to still run later ones, while preserving
 * a "global" exit status.
 */
#define _RUN_TEST( _EC, _NAME, ... )                      \
  {                                                       \
    int __ec = runTest(             ( # _NAME )           \
                      ,               ( test_ ## _NAME )  \
                      __VA_OPT__( , ) __VA_ARGS__         \
                      );                                  \
    if ( __ec != EXIT_SUCCESS ) { _EC = __ec; }           \
  }


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
