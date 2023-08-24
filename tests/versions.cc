/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include "versions.hh"


/* -------------------------------------------------------------------------- */

  bool
test_semverSat1()
{
  std::list<std::string> sats = versions::semverSat( "^4.2.0", {
    "4.0.0", "4.2.0", "4.2.1", "4.3.0", "5.0.0", "3.9.9"
  } );
  return ( sats.size() == 3 ) &&
         ( std::find( sats.begin(), sats.end(), "4.2.0" ) != sats.end() ) &&
         ( std::find( sats.begin(), sats.end(), "4.2.1" ) != sats.end() ) &&
         ( std::find( sats.begin(), sats.end(), "4.3.0" ) != sats.end() );
}


/* -------------------------------------------------------------------------- */

  bool
test_isSemver0()
{
  EXPECT( versions::isSemver( "4.2.0" ) );
  EXPECT( versions::isSemver( "4.2.0-pre" ) );
  EXPECT( ! versions::isSemver( "v4.2.0" ) );
  EXPECT( ! versions::isSemver( "v4.2.0-pre" ) );
  return true;
}


/* -------------------------------------------------------------------------- */

/* Must be `%[Yy]-%m-%d` or `%m-%d-%[Yy]` and may contain trailing characters. */
  bool
test_isDate0()
{
  EXPECT( versions::isDate( "10-25-1917" ) );
  EXPECT( versions::isDate( "1917-10-25" ) );
  EXPECT( ! versions::isDate( "1917-25-10" ) );

  EXPECT( versions::isDate( "10-25-1917-pre" ) );
  EXPECT( versions::isDate( "1917-10-25-pre" ) );
  EXPECT( ! versions::isDate( "1917-25-10-pre" ) );

  EXPECT( versions::isDate( "10-25-17" ) );
  EXPECT( versions::isDate( "17-10-25" ) );
  EXPECT( ! versions::isDate( "22-31-10" ) );

  EXPECT( versions::isDate( "10-25-17-pre" ) );
  EXPECT( versions::isDate( "17-10-25-pre" ) );
  EXPECT( ! versions::isDate( "22-31-10-pre" ) );

  EXPECT( ! versions::isDate( "1917-10-25xxx" ) );

  EXPECT( ! versions::isDate( "10:25:1917" ) );
  EXPECT( ! versions::isDate( "1917:25:10" ) );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_getVersionKind0()
{
  EXPECT_EQ( versions::VK_OTHER,  versions::getVersionKind( "" ) );
  EXPECT_EQ( versions::VK_DATE,   versions::getVersionKind( "10-25-1917" ) );
  EXPECT_EQ( versions::VK_DATE,   versions::getVersionKind( "1917-10-25" ) );
  EXPECT_EQ( versions::VK_DATE,   versions::getVersionKind( "10-25-1917-x" ) );
  EXPECT_EQ( versions::VK_DATE,   versions::getVersionKind( "1917-10-25-x" ) );
  EXPECT_EQ( versions::VK_SEMVER, versions::getVersionKind( "4.2.0" ) );
  EXPECT_EQ( versions::VK_SEMVER, versions::getVersionKind( "4.2.0-pre" ) );
  EXPECT_EQ( versions::VK_OTHER,  versions::getVersionKind( "v4.2.0" ) );
  EXPECT_EQ( versions::VK_OTHER,  versions::getVersionKind( "4.2" ) );
  EXPECT_EQ( versions::VK_OTHER,  versions::getVersionKind( "4" ) );
  return true;
}


/* -------------------------------------------------------------------------- */

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( semverSat1 );
  RUN_TEST( isSemver0 );
  RUN_TEST( isDate0 );
  RUN_TEST( getVersionKind0 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
