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

/* Must be `%Y-%m-%d` or `%m-%d-%Y` and may contain trailing characters. */
  bool
test_isDate0()
{
  EXPECT( versions::isDate( "10-25-1917" ) );
  EXPECT( versions::isDate( "1917-10-25" ) );
  EXPECT( ! versions::isDate( "1917-25-10" ) );

  EXPECT( versions::isDate( "10-25-1917-pre" ) );
  EXPECT( versions::isDate( "1917-10-25-pre" ) );
  EXPECT( ! versions::isDate( "1917-25-10-pre" ) );

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

  bool
test_compareSemversLT0()
{
  /* Compare same version pre-release against release */
  EXPECT( versions::compareSemversLT( "4.1.9-pre", "4.1.9" ) );
  EXPECT( versions::compareSemversLT( "4.1.9-pre", "4.1.9", true ) );
  EXPECT( ! versions::compareSemversLT( "4.1.9", "4.1.9-pre" ) );
  EXPECT( ! versions::compareSemversLT( "4.1.9", "4.1.9-pre", true ) );

  /* Compare next minor pre-release to past minor relese */
  EXPECT( versions::compareSemversLT( "4.2.0-pre", "4.1.9" ) );
  EXPECT( ! versions::compareSemversLT( "4.2.0-pre", "4.1.9", true ) );
  EXPECT( ! versions::compareSemversLT( "4.1.9", "4.2.0-pre" ) );
  EXPECT( versions::compareSemversLT( "4.1.9", "4.2.0-pre", true ) );

  /* Compare next minor release to past minor relese */
  EXPECT( ! versions::compareSemversLT( "4.2.0", "4.1.9" ) );
  EXPECT( ! versions::compareSemversLT( "4.2.0", "4.1.9", true ) );
  EXPECT( versions::compareSemversLT( "4.1.9", "4.2.0" ) );
  EXPECT( versions::compareSemversLT( "4.1.9", "4.2.0", true ) );

  /* Compare next minor pre-release to past minor pre-relese */
  EXPECT( ! versions::compareSemversLT( "4.2.0-pre", "4.1.9-pre" ) );
  EXPECT( ! versions::compareSemversLT( "4.2.0-pre", "4.1.9-pre", true ) );
  EXPECT( versions::compareSemversLT( "4.1.9-pre", "4.2.0-pre" ) );
  EXPECT( versions::compareSemversLT( "4.1.9-pre", "4.2.0-pre", true ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/* NOTE: abbreviated years are split such that 68 -> 2068, and 69 -> 1969. */
  bool
test_compareDatesLT0()
{
  /* Ensure equal dates with different formats are equal */
  EXPECT( versions::compareDatesLT( "1970-10-25", "10-25-1970" ) );
  EXPECT( versions::compareDatesLT( "10-25-1970", "1970-10-25" ) );

  /* Same format comparisons */
  EXPECT( versions::compareDatesLT( "1970-10-25", "1970-10-26" ) );
  EXPECT( ! versions::compareDatesLT( "1970-10-26", "1970-10-25" ) );
  EXPECT( versions::compareDatesLT( "10-25-1970", "10-26-1970" ) );
  EXPECT( ! versions::compareDatesLT( "10-26-1970", "10-25-1970" ) );

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
  RUN_TEST( compareSemversLT0 );
  RUN_TEST( compareDatesLT0 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
