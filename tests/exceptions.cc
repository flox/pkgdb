/* ========================================================================== *
 *
 * @file resolver.cc
 *
 * @brief Tests for `flox::exceptions`.
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

/** @brief Test whatString correctly calls virtual methods. */
bool
test_whatString()
{
  FloxException base( "context" );
  EXPECT_EQ( base.whatString(), "general error: context" );

  command::InvalidArgException derived( "context" );
  FloxException*               derived_ptr = &derived;

  EXPECT_EQ( derived_ptr->whatString(), "invalid argument: context" );

  return true;
}


/* -------------------------------------------------------------------------- */

int
main()
{
  int exitCode = EXIT_SUCCESS;
#define RUN_TEST( ... ) _RUN_TEST( exitCode, __VA_ARGS__ )

  RUN_TEST( whatString );

  return exitCode;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
