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
 * In general tests should clear the database's tables at the top of
 * their function.
 * This allows `throw` and early terminations to exit at arbitrary points
 * without polluting later test cases.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <iostream>
#include <cstdlib>
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
#include "flox/core/util.hh"
#include "flox/core/types.hh"
#include "flox/pkgdb.hh"
#include "flox/pkgdb/query-builder.hh"
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

  static inline void
clearTables( flox::pkgdb::PkgDb & db )
{
  /* Clear DB */
  db.execute_all(
    "DELETE FROM Packages; DELETE FROM AttrSets; DELETE FROM Descriptions"
  );
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
  clearTables( db );

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
  clearTables( db );

  try
    {
      /* Ensure we throw an error for undefined `AttrSet.id' parents. */
      db.addOrGetAttrSetId( "phony", 1 );
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
 * Ensure `PkgDb::hasAttrSet` works regardless of whether `Packages` exist in
 * an `AttrSet`.
 */
  bool
test_hasAttrSet0( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make sure the attr-set exists, and clear it. */
  row_id id = db.addOrGetAttrSetId( "x86_64-linux"
                                  , db.addOrGetAttrSetId( "legacyPackages" )
                                  );
  sqlite3pp::command cmd( db.db
                        , "DELETE FROM Packages WHERE ( parentId = :id )"
                        );
  cmd.bind( ":id", (long long int) id );
  cmd.execute();

  EXPECT( db.hasAttrSet(
            std::vector<std::string> { "legacyPackages", "x86_64-linux" }
          )
        );
  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Ensure `PkgDb::hasAttrSet` works when `Packages` exist in an `AttrSet`
 * such that attribute sets with packages are identified as "Package Sets".
 */
  bool
test_hasAttrSet1( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

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

  EXPECT( db.hasAttrSet(
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
  clearTables( db );

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
  clearTables( db );

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
  clearTables( db );

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
      flox::AttrPath { "legacyPackages", "x86_64-linux", "phony" }
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


/* -------------------------------------------------------------------------- */

  bool
test_descendants0( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  row_id linux =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-linux" } );
  row_id python = db.addOrGetAttrSetId( "python3Packages", linux );
  row_id node   = db.addOrGetAttrSetId( "nodePackages", linux );
  row_id foo    = db.addOrGetAttrSetId( "fooPackages", linux );
  row_id bar    = db.addOrGetAttrSetId( "bar", foo );
  row_id baz    = db.addOrGetAttrSetId( "baz", foo );
  /* Ensure `ORDER BY' works as expected.
   * `quux' should go before `bar'.
   * `karl' should go after `baz'. */
  row_id quux = db.addOrGetAttrSetId( "quuxPackages", linux );
  row_id karl = db.addOrGetAttrSetId( "karl", quux );
  /* Make sure these don't appear */
  db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-darwin" } );

  std::vector<row_id> descendants = db.getDescendantAttrSets( linux );

  EXPECT( descendants ==
          ( std::vector<row_id> { python, node, foo, quux, bar, baz, karl } )
        );

  return true;
}


/* -------------------------------------------------------------------------- */

/* Tests `systems', `name', `pname', `version', and `subtree' filtering. */
  bool
test_PkgQuery0( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make a package */
  row_id linux =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-linux" } );
  row_id desc =
    db.addOrGetDescriptionId( "A program with a friendly greeting" );
  sqlite3pp::command cmd( db.db, R"SQL(
    INSERT INTO Packages (
      parentId, attrName, name, pname, version, semver, outputs, descriptionId
    ) VALUES ( :parentId, 'hello', 'hello-2.12.1', 'hello', '2.12.1', '2.12.1'
             , '["out"]', :descriptionId
             )
  )SQL" );
  cmd.bind( ":parentId",      (long long) linux );
  cmd.bind( ":descriptionId", (long long) desc  );
  if ( flox::pkgdb::sql_rc rc = cmd.execute(); flox::pkgdb::isSQLError( rc ) )
    {
      throw flox::pkgdb::PkgDbException(
        db.dbPath
      , nix::fmt( "Failed to write Package 'hello':(%d) %s"
                , rc
                , db.db.error_msg()
                )
      );
    }
  flox::pkgdb::PkgQueryArgs qargs;
  qargs.systems = std::vector<std::string> { "x86_64-linux" };

  /* Run empty query */
  {
    flox::pkgdb::PkgQuery query( qargs );
    sqlite3pp::query      qry( db.db, query.str().c_str() );
    for ( const auto & [var, val] : query.binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    auto i = qry.begin();
    EXPECT( ( i != qry.end() ) && ( 0 < ( * i ).get<long long>( 0 ) ) );
  }

  /* Run `pname' query */
  {
    qargs.pname = "hello";
    flox::pkgdb::PkgQuery query( qargs );
    qargs.pname = std::nullopt;
    sqlite3pp::query qry( db.db, query.str().c_str() );
    for ( const auto & [var, val] : query.binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    auto i = qry.begin();
    EXPECT( ( i != qry.end() ) && ( 0 < ( * i ).get<long long>( 0 ) ) );
  }

  /* Run `version' query */
  {
    qargs.version = "2.12.1";
    flox::pkgdb::PkgQuery query( qargs );
    qargs.version = std::nullopt;
    sqlite3pp::query qry( db.db, query.str().c_str() );
    for ( const auto & [var, val] : query.binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    auto i = qry.begin();
    EXPECT( ( i != qry.end() ) && ( 0 < ( * i ).get<long long>( 0 ) ) );
  }

  /* Run `name' query */
  {
    qargs.name = "hello-2.12.1";
    flox::pkgdb::PkgQuery query( qargs );
    qargs.name = std::nullopt;
    sqlite3pp::query qry( db.db, query.str().c_str() );
    for ( const auto & [var, val] : query.binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    auto i = qry.begin();
    EXPECT( ( i != qry.end() ) && ( 0 < ( * i ).get<long long>( 0 ) ) );
  }

  /* Run `subtrees' query */
  {
    qargs.subtrees = std::vector<flox::subtree_type> { flox::ST_LEGACY };
    flox::pkgdb::PkgQuery query( qargs );
    qargs.subtrees = std::nullopt;
    sqlite3pp::query qry( db.db, query.str().c_str() );
    for ( const auto & [var, val] : query.binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    auto i = qry.begin();
    EXPECT( ( i != qry.end() ) && ( 0 < ( * i ).get<long long>( 0 ) ) );
  }

  return true;
}


/* -------------------------------------------------------------------------- */

/* Tests `license', `allowBroken', and `allowUnfree' filtering. */
  bool
test_buildPkgQuery1( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make a package */
  row_id linux =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-linux" } );
  row_id desc =
    db.addOrGetDescriptionId( "A program with a friendly greeting/farewell" );
  sqlite3pp::command cmd( db.db, R"SQL(
    INSERT INTO Packages (
      parentId, attrName, name, pname, version, semver, outputs, license
    , broken, unfree, descriptionId
    ) VALUES
      ( :parentId, 'hello', 'hello-2.12.1', 'hello', '2.12.1', '2.12.1'
      , '["out"]', "GPL-3.0-or-later", FALSE, FALSE, :descriptionId
      )
    , ( :parentId, 'goodbye', 'goodbye-2.12.1', 'goodbye', '2.12.1', '2.12.1'
      , '["out"]', NULL, FALSE, TRUE, :descriptionId
      )
    , ( :parentId, 'hola', 'hola-2.12.1', 'hola', '2.12.1', '2.12.1'
      , '["out"]', "BUSL-1.1", FALSE, FALSE, :descriptionId
      )
    , ( :parentId, 'ciao', 'ciao-2.12.1', 'ciao', '2.12.1', '2.12.1'
      , '["out"]', NULL, TRUE, FALSE, :descriptionId
      )
  )SQL" );
  cmd.bind( ":parentId",      (long long) linux );
  cmd.bind( ":descriptionId", (long long) desc  );
  if ( flox::pkgdb::sql_rc rc = cmd.execute();
       flox::pkgdb::isSQLError( rc )
     )
    {
      throw flox::pkgdb::PkgDbException(
        db.dbPath
      , nix::fmt( "Failed to write Packages:(%d) %s"
                , rc
                , db.db.error_msg()
                )
      );
    }
  flox::pkgdb::PkgQueryArgs qargs;
  qargs.systems = std::vector<std::string> { "x86_64-linux" };

  /* Run `allowBroken = false' query */
  {
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 3 );
  }

  /* Run `allowBroken = true' query */
  {
    qargs.allowBroken = true;
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    qargs.allowBroken = false;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 4 );
  }

  /* Run `allowUnfree = true' query */
  {
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 3 );  /* still omits broken */
  }

  /* Run `allowUnfree = false' query */
  {
    qargs.allowUnfree = false;
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    qargs.allowUnfree = true;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 2 );  /* still omits broken as well */
  }

  /* Run `licenses = ["GPL-3.0-or-later", "BUSL-1.1", "MIT"]' query */
  {
    qargs.licenses = std::vector<std::string> {
      "GPL-3.0-or-later", "BUSL-1.1", "MIT"
    };
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    qargs.licenses = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 2 );  /* omits NULL licenses */
  }

  /* Run `licenses = ["BUSL-1.1", "MIT"]' query */
  {
    qargs.licenses = std::vector<std::string> { "BUSL-1.1", "MIT" };
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    qargs.licenses = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 1 );  /* omits NULL licenses */
  }

  return true;
}


/* -------------------------------------------------------------------------- */

/* Tests `match' filtering. */
  bool
test_buildPkgQuery2( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make a package */
  row_id linux =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-linux" } );
  row_id descGreet =
    db.addOrGetDescriptionId( "A program with a friendly hello" );
  row_id descFarewell =
    db.addOrGetDescriptionId( "A program with a friendly farewell" );
  sqlite3pp::command cmd( db.db, R"SQL(
    INSERT INTO Packages (
      parentId, attrName, name, pname, outputs, descriptionId
    ) VALUES
      ( :parentId, 'aHello', 'hello-2.12.1', 'hello', '["out"]', :descGreetId
      )
    , ( :parentId, 'aGoodbye', 'goodbye-2.12.1', 'goodbye'
      , '["out"]', :descFarewellId
      )
    , ( :parentId, 'aHola', 'hola-2.12.1', 'hola', '["out"]', :descGreetId
      )
    , ( :parentId, 'aCiao', 'ciao-2.12.1', 'ciao', '["out"]', :descFarewellId
      )
  )SQL" );
  cmd.bind( ":parentId",        (long long) linux        );
  cmd.bind( ":descGreetId",     (long long) descGreet    );
  cmd.bind( ":descFarewellId",  (long long) descFarewell );
  if ( flox::pkgdb::sql_rc rc = cmd.execute(); flox::pkgdb::isSQLError( rc ) )
    {
      throw flox::pkgdb::PkgDbException(
        db.dbPath
      , nix::fmt( "Failed to write Packages:(%d) %s"
                , rc
                , db.db.error_msg()
                )
      );
    }
  flox::pkgdb::PkgQueryArgs qargs;
  qargs.systems = std::vector<std::string> { "x86_64-linux" };

  /* Run `match = "hello"' query */
  int matchStrengthIdx = -1;
  {
    qargs.match = "hello";
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs, true );
    qargs.match = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( int i = 0; i < qry.column_count(); ++i )
      {
        if ( qry.column_name( i ) == std::string_view( "matchStrength" ) )
          {
            matchStrengthIdx = i;
            break;
          }
      }
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry )
      {
        (void) r;
        ++count;
        flox::pkgdb::match_strength strength =
          (flox::pkgdb::match_strength) r.get<int>( matchStrengthIdx );
        if ( count == 1 )
          {
            EXPECT_EQ( strength, flox::pkgdb::MS_EXACT_PNAME );
          }
        else
          {
            EXPECT_EQ( strength, flox::pkgdb::MS_PARTIAL_DESC );
          }
      }
    EXPECT_EQ( count, (size_t) 2 );
  }

  /* Run `match = "farewell"' query */
  {
    qargs.match = "farewell";
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs, true );
    qargs.match = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry )
      {
        (void) r;
        ++count;
        EXPECT_EQ( r.get<int>( matchStrengthIdx )
                 , flox::pkgdb::MS_PARTIAL_DESC
                 );
      }
    EXPECT_EQ( count, (size_t) 2 );
  }

  /* Run `match = "hel"' query */
  {
    qargs.match = "hel";
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs, true );
    qargs.match = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry )
      {
        (void) r;
        ++count;
        flox::pkgdb::match_strength strength =
          (flox::pkgdb::match_strength) r.get<int>( matchStrengthIdx );
        if ( count == 1 )
          {
            EXPECT_EQ( strength, flox::pkgdb::MS_PARTIAL_PNAME_DESC );
          }
        else
          {
            EXPECT_EQ( strength, flox::pkgdb::MS_PARTIAL_DESC );
          }
      }
    EXPECT_EQ( count, (size_t) 2 );
  }

  /* Run `match = "xxxxx"' query */
  {
    qargs.match = "xxxxx";
    auto [query, binds] = flox::pkgdb::buildPkgQuery( qargs );
    qargs.match = std::nullopt;
    sqlite3pp::query qry( db.db, query.c_str() );
    for ( const auto & [var, val] : binds )
      {
        qry.bind( var.c_str(), val, sqlite3pp::copy );
      }
    size_t count = 0;
    for ( const auto r : qry ) { (void) r; ++count; }
    EXPECT_EQ( count, (size_t) 0 );
  }

  return true;
}


/* -------------------------------------------------------------------------- */

/* Tests `getPackages', particularly `semver' filtering. */
  bool
test_getPackages0( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make a package */
  row_id linux =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-linux" } );
  row_id desc =
    db.addOrGetDescriptionId( "A program with a friendly greeting/farewell" );
  sqlite3pp::command cmd( db.db, R"SQL(
    INSERT INTO Packages (
      parentId, attrName, name, pname, version, semver, outputs, descriptionId
    ) VALUES
      ( :parentId, 'hello0', 'hello-2.12', 'hello', '2.12', '2.12.0'
      , '["out"]', :descriptionId
      )
    , ( :parentId, 'hello1', 'hello-2.12.1', 'hello', '2.12.1', '2.12.1'
      , '["out"]', :descriptionId
      )
    , ( :parentId, 'hello2', 'hello-3', 'hello', '3', '3.0.0'
      , '["out"]', :descriptionId
      )
  )SQL" );
  cmd.bind( ":parentId",      (long long) linux );
  cmd.bind( ":descriptionId", (long long) desc  );
  if ( flox::pkgdb::sql_rc rc = cmd.execute(); flox::pkgdb::isSQLError( rc ) )
    {
      throw flox::pkgdb::PkgDbException(
        db.dbPath
      , nix::fmt( "Failed to write Packages:(%d) %s"
                , rc
                , db.db.error_msg()
                )
      );
    }

  flox::pkgdb::PkgQueryArgs qargs;
  qargs.systems = std::vector<std::string> { "x86_64-linux" };

  /* Run `semver = "^2"' query */
  {
    qargs.semver = { "^2" };
    size_t count = db.getPackages( qargs ).size();
    qargs.semver = std::nullopt;
    EXPECT_EQ( count, (size_t) 2 );
  }

  /* Run `semver = "^3"' query */
  {
    qargs.semver = { "^3" };
    size_t count = db.getPackages( qargs ).size();
    qargs.semver = std::nullopt;
    EXPECT_EQ( count, (size_t) 1 );
  }

  /* Run `semver = "^2.13"' query */
  {
    qargs.semver = { "^2.13" };
    size_t count = db.getPackages( qargs ).size();
    qargs.semver = std::nullopt;
    EXPECT_EQ( count, (size_t) 0 );
  }

  return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Tests `getPackages', particularly `stability', `subtree`, and
 * `system` ordering. */
  bool
test_getPackages1( flox::pkgdb::PkgDb & db )
{
  clearTables( db );

  /* Make a package */
  row_id stableLinux = db.addOrGetAttrSetId(
    flox::AttrPath { "catalog", "x86_64-linux", "stable" }
  );
  row_id unstableLinux = db.addOrGetAttrSetId(
    flox::AttrPath { "catalog", "x86_64-linux", "unstable" }
  );
  row_id packagesLinux =
    db.addOrGetAttrSetId( flox::AttrPath { "packages", "x86_64-linux" } );
  row_id legacyDarwin =
    db.addOrGetAttrSetId( flox::AttrPath { "legacyPackages", "x86_64-darwin" } );
  row_id packagesDarwin =
    db.addOrGetAttrSetId( flox::AttrPath { "packages", "x86_64-darwin" } );

  row_id desc =
    db.addOrGetDescriptionId( "A program with a friendly greeting/farewell" );

  sqlite3pp::command cmd( db.db, R"SQL(
    INSERT INTO Packages (
      id, parentId, attrName, name, outputs, descriptionId
    ) VALUES
      ( 1, :stableLinuxId,    'hello', 'hello', '["out"]', :descriptionId )
    , ( 2, :unstableLinuxId,  'hello', 'hello', '["out"]', :descriptionId )
    , ( 3, :packagesLinuxId,  'hello', 'hello', '["out"]', :descriptionId )
    , ( 4, :legacyDarwinId,   'hello', 'hello', '["out"]', :descriptionId )
    , ( 5, :packagesDarwinId, 'hello', 'hello', '["out"]', :descriptionId )
  )SQL" );
  cmd.bind( ":descriptionId",    (long long) desc  );
  cmd.bind( ":stableLinuxId",    (long long) stableLinux );
  cmd.bind( ":unstableLinuxId",  (long long) unstableLinux );
  cmd.bind( ":packagesLinuxId",  (long long) packagesLinux );
  cmd.bind( ":legacyDarwinId",   (long long) legacyDarwin );
  cmd.bind( ":packagesDarwinId", (long long) packagesDarwin );
  if ( flox::pkgdb::sql_rc rc = cmd.execute(); flox::pkgdb::isSQLError( rc ) )
    {
      throw flox::pkgdb::PkgDbException(
        db.dbPath
      , nix::fmt( "Failed to write Packages:(%d) %s"
                , rc
                , db.db.error_msg()
                )
      );
    }

  flox::pkgdb::PkgQueryArgs qargs;
  qargs.systems = std::vector<std::string> {};

  /* Test `subtrees` ordering */
  {
    qargs.systems  = std::vector<std::string> { "x86_64-darwin" };
    qargs.subtrees = std::vector<flox::subtree_type> {
      flox::ST_PACKAGES, flox::ST_LEGACY
    };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 5, 4 } ) );
    qargs.subtrees = std::vector<flox::subtree_type> {
      flox::ST_LEGACY, flox::ST_PACKAGES
    };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 4, 5 } ) );
    qargs.subtrees = std::nullopt;
    qargs.systems  = std::vector<std::string> {};
  }

  /* Test `systems` ordering */
  {
    qargs.subtrees = std::vector<flox::subtree_type> { flox::ST_PACKAGES };
    qargs.systems  = std::vector<std::string> {
      "x86_64-linux", "x86_64-darwin"
    };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 3, 5 } ) );
    qargs.systems = std::vector<std::string> {
      "x86_64-darwin", "x86_64-linux"
    };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 5, 3 } ) );
    qargs.systems  = std::vector<std::string> {};
    qargs.subtrees = std::nullopt;
  }

  /* Test `stabilities` ordering */
  {
    qargs.subtrees    = std::vector<flox::subtree_type> { flox::ST_CATALOG };
    qargs.systems     = std::vector<std::string> { "x86_64-linux" };
    qargs.stabilities = std::vector<std::string> { "stable", "unstable" };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 1, 2 } ) );
    qargs.stabilities = std::vector<std::string> { "unstable", "stable" };
    EXPECT( db.getPackages( qargs ) == ( std::vector<row_id> { 2, 1 } ) );
    qargs.stabilities = std::nullopt;
    qargs.systems     = std::vector<std::string> {};
    qargs.subtrees    = std::nullopt;
  }

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

    RUN_TEST( hasAttrSet0, db );
    RUN_TEST( hasAttrSet1, db );

    RUN_TEST( getAttrSetId0, db );

    RUN_TEST( getAttrSetPath0, db );

    RUN_TEST( hasPackage0, db );

    RUN_TEST( descriptions0, db );

    RUN_TEST( descendants0, db );

    RUN_TEST( PkgQuery0, db );

    RUN_TEST( buildPkgQuery1, db );
    RUN_TEST( buildPkgQuery2, db );

    RUN_TEST( getPackages0, db );
    RUN_TEST( getPackages1, db );

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
