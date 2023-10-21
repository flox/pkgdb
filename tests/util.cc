/* ========================================================================== *
 *
 * @file util.cc
 *
 * @brief Tests for `flox` utility interfaces.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdlib>
#include <iostream>

#include <gtest/gtest.h>

#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, simple )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.b.c" )
          == ( flox::AttrPath { "a", "b", "c" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, singleQuotes )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.'b.c'.d" )
          == ( flox::AttrPath { "a", "b.c", "d" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, doubleQuotes )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.\"b.c\".d" )
          == ( flox::AttrPath { "a", "b.c", "d" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, nestedQuotes )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.\"b.'c.d'.e\".f" )
          == ( flox::AttrPath { "a", "b.'c.d'.e", "f" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, escapeQuote )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.\\\"b.c" )
          == ( flox::AttrPath { "a", "\"b", "c" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, nestedEscapeQuotes )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.'\"b'.c" )
          == ( flox::AttrPath { "a", "\"b", "c" } ) );
}


/* -------------------------------------------------------------------------- */

TEST( splitAttrPath, nestedEscapeBackslash )
{
  EXPECT_TRUE( flox::splitAttrPath( "a.\\\\\\..c" )
          == ( flox::AttrPath { "a", "\\.", "c" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test conversion of variants with 2 options. */
TEST( variantJSON, two )
{
  using Trivial = std::variant<bool, std::string>;

  Trivial        tbool = true;
  Trivial        tstr  = "Howdy";
  nlohmann::json jto   = tbool;

  EXPECT_EQ( jto, true );

  jto = tstr;
  EXPECT_EQ( jto, "Howdy" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test conversion of variants with 3 options. */
TEST( variantJSON, three )
{
  using Trivial = std::variant<int, bool, std::string>;

  Trivial        tint  = 420;
  Trivial        tbool = true;
  Trivial        tstr  = "Howdy";
  nlohmann::json jto   = tint;

  EXPECT_EQ( jto, 420 );

  jto = tbool;
  EXPECT_EQ( jto, true );

  jto = tstr;
  EXPECT_EQ( jto, "Howdy" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test conversion of variants with 2 options in a vector. */
TEST( variantJSON, vector )
{
  using Trivial = std::variant<bool, std::string>;

  std::vector<Trivial> tvec = { true, "Howdy" };

  nlohmann::json jto = tvec;

  EXPECT_TRUE( jto.is_array() );
  EXPECT_EQ( jto.at( 0 ), true );
  EXPECT_EQ( jto.at( 1 ), "Howdy" );

  std::vector<Trivial> back = jto;
  EXPECT_EQ( back.size(), std::size_t( 2 ) );

  EXPECT_TRUE( std::holds_alternative<bool>( back.at( 0 ) ) );
  EXPECT_EQ( std::get<bool>( back.at( 0 ) ), std::get<bool>( tvec.at( 0 ) ) );

  EXPECT_TRUE( std::holds_alternative<std::string>( back.at( 1 ) ) );
  EXPECT_EQ( std::get<std::string>( back.at( 1 ) ),
             std::get<std::string>( tvec.at( 1 ) ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test conversion of variants with 3 options in a vector. */
TEST( variantJSON, threeVector )
{
  /* NOTE: `bool` MUST come before `int` to avoid coercion!
   * `std::string` always has to go last. */
  using Trivial = std::variant<bool, int, std::string>;

  std::vector<Trivial> tvec = { true, "Howdy", 420 };

  nlohmann::json jto = tvec;

  EXPECT_TRUE( jto.is_array() );
  EXPECT_EQ( jto.at( 0 ), true );
  EXPECT_EQ( jto.at( 1 ), "Howdy" );
  EXPECT_EQ( jto.at( 2 ), 420 );

  std::vector<Trivial> back = jto;
  EXPECT_EQ( back.size(), std::size_t( 3 ) );

  EXPECT_TRUE( std::holds_alternative<bool>( back.at( 0 ) ) );
  EXPECT_EQ( std::get<bool>( back.at( 0 ) ), std::get<bool>( tvec.at( 0 ) ) );

  EXPECT_TRUE( std::holds_alternative<std::string>( back.at( 1 ) ) );
  EXPECT_EQ( std::get<std::string>( back.at( 1 ) ),
             std::get<std::string>( tvec.at( 1 ) ) );

  EXPECT_TRUE( std::holds_alternative<int>( back.at( 2 ) ) );
  EXPECT_EQ( std::get<int>( back.at( 2 ) ), std::get<int>( tvec.at( 2 ) ) );
}


/* -------------------------------------------------------------------------- */

TEST(hasPrefix0, simple)
{
  EXPECT_TRUE( flox::hasPrefix( "foo", "foobar" ) );
  EXPECT_FALSE( flox::hasPrefix( "bar", "foobar" ) );
  EXPECT_FALSE( flox::hasPrefix( "foobar", "foo" ) );
}


/* -------------------------------------------------------------------------- */

int
main( int argc, char * argv[] )
{
  testing::InitGoogleTest( & argc, argv );
  testing::TestEventListeners & listeners =
    testing::UnitTest::GetInstance()->listeners();
  /* Delete the default listener. */
  delete listeners.Release( listeners.default_result_printer() );
  listeners.Append( new tap::TapListener() );
  return RUN_ALL_TESTS();
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
