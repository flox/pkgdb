/* ========================================================================== *
 *
 * @file pkgdb/read.cc
 *
 * @brief Implementations for reading a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <memory>

#include "flox/pkgdb/read.hh"
#include "flox/flake-package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

  bool
isSQLError( int rc )
{
  switch ( rc )
  {
    case SQLITE_OK:   return false; break;
    case SQLITE_ROW:  return false; break;
    case SQLITE_DONE: return false; break;
    default:          return true;  break;
  }
}



/* -------------------------------------------------------------------------- */

  std::string
genPkgDbName( const Fingerprint & fingerprint )
{
  nix::Path   cacheDir = nix::getCacheDir() + "/flox/pkgdb-v0";
  std::string fpStr    = fingerprint.to_string( nix::Base16, false );
  nix::Path   dbPath   = cacheDir + "/" + fpStr + ".sqlite";
  return dbPath;
}


/* -------------------------------------------------------------------------- */

  void
PkgDbReadOnly::loadLockedFlake()
{
  sqlite3pp::query qry(
    this->db
  , "SELECT fingerprint, string, attrs FROM LockedFlake LIMIT 1"
  );
  auto r = * qry.begin();
  std::string fingerprintStr = r.get<std::string>( 0 );
  nix::Hash   fingerprint    =
    nix::Hash::parseNonSRIUnprefixed( fingerprintStr, nix::htSHA256 );
  this->lockedRef.string = r.get<std::string>( 1 );
  this->lockedRef.attrs  = nlohmann::json::parse( r.get<std::string>( 2 ) );
  /* Check to see if our fingerprint is already known.
   * If it isn't load it, otherwise assert it matches. */
  if ( this->fingerprint == nix::Hash( nix::htSHA256 ) )
    {
      this->fingerprint = fingerprint;
    }
  else if ( this->fingerprint != fingerprint )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "database '%s' fingerprint '%s' does not match expected '%s'"
                , this->dbPath
                , fingerprintStr
                , this->fingerprint.to_string( nix::Base16, false )
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDbReadOnly::getDbVersion()
{
  sqlite3pp::query qry(
    this->db
  , "SELECT version FROM DbVersions WHERE name = 'pkgdb_schema' LIMIT 1"
  );
  return ( * qry.begin() ).get<std::string>( 0 );
}


/* -------------------------------------------------------------------------- */

  bool
PkgDbReadOnly::hasAttrSet( const flox::AttrPath & path )
{
  /* Lookup the `AttrName.id' ( if one exists ) */
  row_id id = 0;
  for ( const auto & a : path )
    {
      sqlite3pp::query qryId(
        this->db
      , "SELECT id FROM AttrSets "
        "WHERE ( attrName = :attrName ) AND ( parent = :parent )"
      );
      qryId.bind( ":attrName", a, sqlite3pp::copy );
      qryId.bind( ":parent", (long long) id );
      auto i = qryId.begin();
      if ( i == qryId.end() ) { return false; }  /* No such path. */
      id = ( * i ).get<long long>( 0 );
    }
  return true;
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDbReadOnly::getDescription( row_id descriptionId )
{
  if ( descriptionId == 0 ) { return ""; }
  /* Lookup the `Description.id' ( if one exists ) */
  sqlite3pp::query qryId(
    this->db
  , "SELECT description FROM Descriptions WHERE id = :descriptionId"
  );
  qryId.bind( ":descriptionId", (long long) descriptionId );
  auto i = qryId.begin();
  /* Handle no such path. */
  if ( i == qryId.end() )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "No such Descriptions.id %llu.", descriptionId )
      );
    }
  return ( * i ).get<std::string>( 0 );
}


/* -------------------------------------------------------------------------- */

  bool
PkgDbReadOnly::hasPackage( const flox::AttrPath & path )
{
  flox::AttrPath parent;
  for ( size_t i = 0; i < ( path.size() - 1 ); ++i )
    {
      parent.emplace_back( path[i] );
    }

  /* Make sure there are actually packages in the set. */
  row_id id = this->getAttrSetId( parent );
  sqlite3pp::query qryPkgs(
    this->db
  , "SELECT id FROM Packages WHERE ( parentId = :parentId ) "
    "AND ( attrName = :attrName ) LIMIT 1"
  );
  qryPkgs.bind( ":parentId", (long long) id );
  qryPkgs.bind( ":attrName", std::string( path.back() ), sqlite3pp::copy );
  return ( * qryPkgs.begin() ).get<int>( 0 ) != 0;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDbReadOnly::getAttrSetId( const flox::AttrPath & path )
{
  /* Lookup the `AttrName.id' ( if one exists ) */
  row_id id = 0;
  for ( const auto & a : path )
    {
      sqlite3pp::query qryId(
        this->db
      , "SELECT id FROM AttrSets "
        "WHERE ( attrName = :attrName ) AND ( parent = :parent ) LIMIT 1"
      );
      qryId.bind( ":attrName", a, sqlite3pp::copy );
      qryId.bind( ":parent", (long long) id );
      auto i = qryId.begin();
      /* Handle no such path. */
      if ( i == qryId.end() )
        {
          throw PkgDbException(
            this->dbPath
          , nix::fmt( "No such AttrSet '%s'."
                    , nix::concatStringsSep( ".", path )
                    )
          );
        }
      id = ( * i ).get<long long>( 0 );
    }

  return id;
}


/* -------------------------------------------------------------------------- */

  flox::AttrPath
PkgDbReadOnly::getAttrSetPath( row_id id )
{
  if ( id == 0 ) { return {}; }
  std::list<std::string> path;
  while ( id != 0 )
    {
      sqlite3pp::query qry(
        this->db
      , "SELECT parent, attrName FROM AttrSets WHERE ( id = :id )"
      );
      qry.bind( ":id", (long long) id );
      auto i = qry.begin();
      /* Handle no such path. */
      if ( i == qry.end() )
        {
          throw PkgDbException(
            this->dbPath
          , nix::fmt( "No such `AttrSet.id' %llu.", id )
          );
        }
      id = ( * i ).get<long long>( 0 );
      path.push_front( ( * i ).get<std::string>( 1 ) );
    }
  return flox::AttrPath { std::make_move_iterator( std::begin( path ) )
                        , std::make_move_iterator( std::end( path ) )
                        };
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDbReadOnly::getPackageId( const flox::AttrPath & path )
{
  /* Lookup the `AttrName.id' of parent ( if one exists ) */
  flox::AttrPath parentPath = path;
  parentPath.pop_back();

  row_id parent = this->getAttrSetId( parentPath );

  sqlite3pp::query qry(
    this->db
  , "SELECT id FROM Packages WHERE "
    "( parentId = :parentId ) AND ( attrName = :attrName )"
  );
  qry.bind( ":parentId", (long long) parent );
  qry.bind( ":attrName", path.back(), sqlite3pp::copy );
  auto i = qry.begin();
  /* Handle no such path. */
  if ( i == qry.end() )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "No such package %s.", nix::concatStringsSep( ".", path ) )
      );
    }
  return ( * i ).get<long long>( 0 );
}


/* -------------------------------------------------------------------------- */

  flox::AttrPath
PkgDbReadOnly::getPackagePath( row_id id )
{
  if ( id == 0 ) { return {}; }
  sqlite3pp::query qry(
    this->db
  , "SELECT parentId, attrName FROM Packages WHERE ( id = :id )"
  );
  qry.bind( ":id", (long long) id );
  auto i = qry.begin();
  /* Handle no such path. */
  if ( i == qry.end() )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "No such `Packages.id' %llu.", id )
      );
    }
  flox::AttrPath path = this->getAttrSetPath( ( * i ).get<long long>( 0 ) );
  path.emplace_back( ( * i ).get<std::string>( 1 ) );
  return path;
}


/* -------------------------------------------------------------------------- */


  std::vector<row_id>
PkgDbReadOnly::getDescendantAttrSets( row_id root )
{
  sqlite3pp::query qry(
    this->db
  , R"SQL(
      WITH RECURSIVE Tree AS (
        SELECT id, parent, 0 as depth FROM AttrSets WHERE ( id = :root )
        UNION ALL SELECT O.id, O.parent, ( Parent.depth + 1 ) AS depth
        FROM AttrSets O JOIN Tree AS Parent ON ( Parent.id = O.parent )
      ) SELECT C.id FROM Tree AS C
      JOIN AttrSets AS Parent ON ( C.parent = Parent.id )
      WHERE ( C.id != :root ) ORDER BY C.depth, C.parent
    )SQL"
  );
  qry.bind( ":root", (long long) root );
  std::vector<row_id> descendants;
  for ( auto row : qry )
    {
      descendants.push_back( row.get<long long>( 0 ) );
    }
  return descendants;
}


/* -------------------------------------------------------------------------- */

  std::optional<size_t>
distanceFromMatch( Package & pkg, std::string match )
{
  if ( match.empty() ) { return std::nullopt; }

  std::string pname = pkg.getPname();
  // TODO match on attrName. That's not currently possible because attrName is
  // meaningful for flakes, but for catalogs, only the attrName of parent is
  // meaningful (attrName is a version string).

  // Don't give description any weight if pname matches exactly. It's not
  // especially meaningful if a description mentions its own name.
  if ( pname == match )
    {
      // pname matches exactly
      return 0;
    }

  bool pnameMatches = (pname.find(match) != std::string::npos);
  auto description = pkg.getDescription();
  bool descriptionMatches = (description.has_value() && description->find(match) != std::string::npos);

  if ( pnameMatches ) {
    if ( descriptionMatches )
      {
        // pname and description match
        return 1;
      }
    // only pname matches
    return 2;
  }

  if ( descriptionMatches )
    {
      // only description matches
      return 3;
    }
  // nothing matches
  return 4;
}

/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
