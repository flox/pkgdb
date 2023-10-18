/* ========================================================================== *
 *
 * @file resolver.cc
 *
 * @brief Tests for `flox::exceptions`.
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdlib>
#include <iostream>

#include "flox/core/command.hh"
#include "flox/core/exceptions.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace flox;

/* -------------------------------------------------------------------------- */

/** @brief Test what_string correctly calls virtual methods. */
bool
test_what_string()
{
  FloxException base( "context" );
  EXPECT_EQ( base.what_string(), "general error: context" );

  command::InvalidArgException derived( "context" );
  FloxException*               derived_ptr = &derived;

  EXPECT_EQ( derived_ptr->what_string(), "invalid argument: context" );

  return true;
}


/* -------------------------------------------------------------------------- */

int
main()
{
  int exitCode = EXIT_SUCCESS;
#define RUN_TEST( ... ) _RUN_TEST( exitCode, __VA_ARGS__ )

  /* --------------------------------------------------------------------------
   */

  {

    RUN_TEST( what_string );
  }

  return exitCode;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
