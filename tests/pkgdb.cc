/* ========================================================================== *
 *
 * @file pkgdb.cc
 *
 * @brief Tests for `flox::pkgdb::PkgDb` interfaces.
 *
 * NOTE: These tests may be order dependant simply because each test case shares
 *       a single database.
 *       Having said that we make a concerted effort to avoid dependence on past
 *       test state by doing things like clearing tables in test cases where
 *       it may be relevant to an action we're about to test.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <iostream>
#include <stdlib.h>
#include <list>
#include <assert.h>
#include <queue>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>
#include <sqlite3pp.hh>

#include "flox/flox-flake.hh"
#include "flox/util.hh"
#include "pkgdb.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using flox::pkgdb::row_id;


/* -------------------------------------------------------------------------- */

  static row_id
getRowCount( flox::pkgdb::PkgDb & db, const std::string table )
{
  std::string qryS = "SELECT COUNT( * ) FROM ";
  qryS += table;
  sqlite3pp::query qry( db.db, qryS.c_str() );
  return ( * qry.begin() ).get<long long int>( 0 );
}


/* -------------------------------------------------------------------------- */

/**
 * Test ability to add `AttrSet` rows.
 * This test should run before all others since it essentially expects
 * `AttrSets` to be empty.
 */
  bool
test_addOrGetAttrSetId0( flox::pkgdb::PkgDb & db )
{
  /* Make sure `AttrSets` is empty. */
  row_id startId = getRowCount( db, "AttrSets" );
  EXPECT_EQ( startId, (row_id) 0 );

  /* Add two `AttrSets` */
  row_id id = db.addOrGetAttrSetId( "legacyPackages" );
  EXPECT_EQ( startId + 1, id );

  id = db.addOrGetAttrSetId( "x86_64-linux", id );
  EXPECT_EQ( startId + 2, id );

  return true;
}


/* -------------------------------------------------------------------------- */

/** Ensure we throw an error for undefined `AttrSet.id' parents. */
  bool
test_addOrGetAttrSetId1( flox::pkgdb::PkgDb & db )
{
  row_id startId = getRowCount( db, "AttrSets" );
  try
    {
      /* Ensure we throw an error for undefined `AttrSet.id' parents. */
      db.addOrGetAttrSetId( "phony", startId + 99999999 );
      return false;
    }
  catch( const flox::pkgdb::PkgDbException & e ) { /* Expected */ }
  catch( const std::exception & e )
    {
      std::cerr << e.what() << std::endl;
      return false;
    }
  return true;
}


/* -------------------------------------------------------------------------- */

/** Ensure database version matches our header's version */
  bool
test_getDbVersion0( flox::pkgdb::PkgDb & db )
{
  EXPECT_EQ( db.getDbVersion(), FLOX_PKGDB_SCHEMA_VERSION );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Ensure `PkgDb::hasPackageSet` checks that `Packages` exist in an `AttrSet`
 * such that attribute sets with no packages are not identified as
 * "Package Sets".
 */
  bool
test_hasPackageSet0( flox::pkgdb::PkgDb & db )
{
  /* Make sure the attr-set exists, and clear it. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  sqlite3pp::command cmd( db.db
                        , "DELETE FROM Packages WHERE ( parentId = :id )"
                        );
  cmd.bind( ":id", (long long int) id );
  cmd.execute();

  EXPECT( ! db.hasPackageSet(
              std::vector<std::string> { "legacyPackages", "x86_64-linux" }
            )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Ensure `PkgDb::hasPackageSet` checks that `Packages` exist in an `AttrSet`
 * such that attribute sets with packages are identified as "Package Sets".
 */
  bool
test_hasPackageSet1( flox::pkgdb::PkgDb & db )
{
  /* Make sure the attr-set exists. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  /* Add a minimal package with this `id` as its parent. */
  sqlite3pp::command cmd(
    db.db
  , R"SQL(
      INSERT OR IGNORE INTO Packages ( parentId, attrName, name, outputs )
      VALUES ( :id, 'phony', 'phony', '["out"]' )
    )SQL"
  );
  cmd.bind( ":id", (long long int) id );
  cmd.execute();

  EXPECT( db.hasPackageSet(
            std::vector<std::string> { "legacyPackages", "x86_64-linux" }
          )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Ensure the `row_id` returned when adding an `AttrSet` matches the one
 * returned by @a flox::pkgdb::PkgDb::getAttrSetId.
 */
  bool
test_getAttrSetId0( flox::pkgdb::PkgDb & db )
{
  /* Make sure the attr-set exists. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  EXPECT_EQ( id
           , db.getAttrSetId(
               std::vector<std::string> { "legacyPackages", "x86_64-linux" }
             )
           );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Ensure we properly reconstruct an attribute path from the `AttrSets` table.
 */
  bool
test_getAttrSetPath0( flox::pkgdb::PkgDb & db )
{
  /* Make sure the attr-set exists. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  std::vector<std::string> path { "legacyPackages", "x86_64-linux" };
  EXPECT( path == db.getAttrSetPath( id ) );
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_hasPackage0( flox::pkgdb::PkgDb & db )
{
  /* Make sure the attr-set exists. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  /* Add a minimal package with this `id` as its parent. */
  sqlite3pp::command cmd(
    db.db
  , R"SQL(
      INSERT OR IGNORE INTO Packages ( parentId, attrName, name, outputs )
      VALUES ( :id, 'phony', 'phony', '["out"]' )
    )SQL"
  );
  cmd.bind( ":id", (long long) id );
  cmd.execute();

  EXPECT(
    db.hasPackage(
      std::vector<std::string> { "legacyPackages", "x86_64-linux", "phony" }
    )
  );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Tests `addOrGetDesciptionId` and `getDescription`.
 */
  bool
test_descriptions0( flox::pkgdb::PkgDb & db )
{
  row_id id = db.addOrGetDescriptionId( "Hello, World!" );
  /* Ensure we get the same `id`. */
  EXPECT_EQ( id, db.addOrGetDescriptionId( "Hello, World!" ) );
  /* Ensure we get back our original string. */
  EXPECT_EQ( "Hello, World!", db.getDescription( id ) );
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

  /* Initialize `nix' */
  flox::NixState nstate( verbosity );


/* -------------------------------------------------------------------------- */

  auto [fd, path] = nix::createTempFile( "test-pkgdb.sql" );
  fd.close();

  nix::FlakeRef ref = nix::parseFlakeRef( nixpkgsRef );

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "fetching flake '%s'", ref.to_string() )
  );
  flox::FloxFlake flake( (nix::ref<nix::EvalState>) nstate.state, ref );
  nix::logger->stopActivity( act.id );


/* -------------------------------------------------------------------------- */

  {

    flox::pkgdb::PkgDb db( flake.lockedFlake, path );

    RUN_TEST( addOrGetAttrSetId0, db );
    RUN_TEST( addOrGetAttrSetId1, db );

    RUN_TEST( getDbVersion0, db );

    RUN_TEST( hasPackageSet0, db );
    RUN_TEST( hasPackageSet1, db );

    RUN_TEST( getAttrSetId0, db );

    RUN_TEST( getAttrSetPath0, db );

    RUN_TEST( hasPackage0, db );

    RUN_TEST( descriptions0, db );

  }


/* -------------------------------------------------------------------------- */

  /* XXX: You may find it useful to preserve the file and print it for some
   *      debugging efforts. */
  std::filesystem::remove( path );
  //std::cerr << path << std::endl;


/* -------------------------------------------------------------------------- */

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
