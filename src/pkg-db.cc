/* ========================================================================== *
 *
 * @file pkg-db.cc
 *
 * @brief Implementations for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#include "pkg-db.hh"

#include "sql/versions.hh"
#include "sql/input.hh"
#include "sql/package-sets.hh"
#include "sql/packages.hh"
//#include "sql/tmp-paths.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
  std::string
getPkgDbName( const Fingerprint & fingerprint )
{
  nix::Path   cacheDir = nix::getCacheDir() + "/flox/pkgdb-v0";
  std::string fpStr    = fingerprint.to_string( nix::Base16, false );
  nix::Path   dbPath   = cacheDir + "/" + fpStr + ".sqlite";
  return dbPath;
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::loadLockedRef()
{
  sqlite3pp::query qry(
    this->db
  , "SELECT string, attrs FROM LockedFlake LIMIT 1"
  );
  auto r = * qry.begin();
  this->lockedRef.string = r.get<std::string>( 0 );
  this->lockedRef.attrs  = nlohmann::json::parse( r.get<std::string>( 1 ) );
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::writeInput()
{
  sqlite3pp::command cmd(
    this->db
  , "INSERT OR IGNORE INTO LockedFlake ( fingerprint, string, attrs ) VALUES "
    "  ( :fingerprint, :string, :attrs )"
  );
  std::string fpStr = fingerprint.to_string( nix::Base16, false );
  cmd.bind( ":fingerprint", fpStr, sqlite3pp::copy );
  cmd.bind( ":string", this->lockedRef.string, sqlite3pp::nocopy );
  cmd.bind( ":attrs",  this->lockedRef.attrs.dump(), sqlite3pp::copy );
  cmd.execute();
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::initTables()
{
  this->execute( sql_versions );
  this->execute( sql_input );
  this->execute( sql_packageSets );
  this->execute( sql_packages );
  static const char * stmtVersions =
    "INSERT OR IGNORE INTO DbVersions ( name, version ) VALUES "
    "  ( 'pkgdb',        '" FLOX_PKGDB_VERSION        "' )"
    ", ( 'pkgdb_schema', '" FLOX_PKGDB_SCHEMA_VERSION "' )";
  this->execute( stmtVersions );
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDb::getDbVersion()
{
  sqlite3pp::query qry(
    this->db
  , "SELECT version FROM DbVersions WHERE name = 'pkgdb_schema' LIMIT 1"
  );
  return ( * qry.begin() ).get<std::string>( 0 );
}


/* -------------------------------------------------------------------------- */

  bool
PkgDb::hasPackageSet( const AttrPathV & path )
{
  /* Lookup the `PackageSet.id' ( if one exists ) */
  sqlite3pp::query qryId(
    this->db
  , "SELECT pathId FROM PackageSets WHERE path = :path"
  );
  qryId.bind( ":path", nlohmann::json( path ).dump(), sqlite3pp::copy );
  auto i = qryId.begin();
  if ( i == qryId.end() ) { return false; }  /* No such path. */

  /* Make sure there are actually packages in the set. */
  row_id id = ( * i ).get<long long int>( 0 );
  sqlite3pp::query qryPkgs(
    this->db
  , "SELECT COUNT( id ) FROM Packages WHERE parentId = :parentId"
  );
  qryPkgs.bind( ":parentId", (long long int) id );
  return 0 < ( * qryPkgs.begin() ).get<int>( 0 );
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::getPackageSetId( const AttrPathV & path )
{
  /* Lookup the `PackageSet.id' ( if one exists ) */
  sqlite3pp::query qryId(
    this->db
  , "SELECT pathId FROM PackageSets WHERE path = :path"
  );
  qryId.bind( ":path", nlohmann::json( path ).dump(), sqlite3pp::copy );
  auto i = qryId.begin();
  /* Handle no such path. */
  if ( i == qryId.end() )
    {
      std::string msg( "No such PackageSet '" );
      bool first = true;
      for ( const auto p : path )
        {
          if ( first ) { first = false; } else { msg += '.'; }
          msg += p;
        }
      msg += "'.";
      throw PkgDbException( msg );
    }
  return ( * i ).get<long long int>( 0 );
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDb::getDescription( row_id descriptionId )
{
  // TODO
  return "";
}


/* -------------------------------------------------------------------------- */

  bool
PkgDb::hasPackage( const AttrPathV & path )
{
  // TODO
  return false;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetPackageSetId( const AttrPathV & path )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetDescriptionId( std::string_view description )
{
  // TODO
  return 0;
}

/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addPackage( row_id           parentId
                 , std::string_view attrName
                 , Cursor           cursor
                 , bool             replace
                 )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
