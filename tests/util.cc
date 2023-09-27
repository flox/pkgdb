/* ========================================================================== *
 *
 * @file util.cc
 *
 * @brief Tests for `flox` utility interfaces.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>

#include "test.hh"
#include "flox/core/util.hh"
#include "flox/core/types.hh"


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath0()
{
  EXPECT( flox::splitAttrPath( "a.b.c" ) ==
          ( flox::AttrPath { "a", "b", "c" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath1()
{
  EXPECT( flox::splitAttrPath( "a.'b.c'.d" ) ==
          ( flox::AttrPath { "a", "b.c", "d" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath2()
{
  EXPECT( flox::splitAttrPath( "a.\"b.c\".d" ) ==
          ( flox::AttrPath { "a", "b.c", "d" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath3()
{
  EXPECT( flox::splitAttrPath( "a.\"b.'c.d'.e\".f" ) ==
          ( flox::AttrPath { "a", "b.'c.d'.e", "f" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath4()
{
  EXPECT( flox::splitAttrPath( "a.\\\"b.c" ) ==
          ( flox::AttrPath { "a", "\"b", "c" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath5()
{
  EXPECT( flox::splitAttrPath( "a.'\"b'.c" ) ==
          ( flox::AttrPath { "a", "\"b", "c" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_splitAttrPath6()
{
  EXPECT( flox::splitAttrPath( "a.\\\\\\..c" ) ==
          ( flox::AttrPath { "a", "\\.", "c" } )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_variantJSON0()
{
  using Trivial = std::variant<int, bool, std::string>;

  Trivial tint       = 420;
  Trivial tbool      = true;
  Trivial tstr       = "Howdy";
  nlohmann::json jto = tint;

  EXPECT_EQ( jto, 420 );

  jto = tbool;
  EXPECT_EQ( jto, true );

  jto = tstr;
  EXPECT_EQ( jto, "Howdy" );

  return true;
}


/* -------------------------------------------------------------------------- */

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  RUN_TEST( splitAttrPath0 );
  RUN_TEST( splitAttrPath1 );
  RUN_TEST( splitAttrPath2 );
  RUN_TEST( splitAttrPath3 );
  RUN_TEST( splitAttrPath4 );
  RUN_TEST( splitAttrPath5 );
  RUN_TEST( splitAttrPath6 );

  RUN_TEST( variantJSON0 );

/* -------------------------------------------------------------------------- */

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
