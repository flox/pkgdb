/* ========================================================================== *
 *
 * @file pkgdb.cc
 *
 * @brief Implementations for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <memory>

#include "pkgdb.hh"
#include "flox/flake-package.hh"

#include "sql/versions.hh"
#include "sql/input.hh"
#include "sql/package-sets.hh"
#include "sql/packages.hh"



/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

  static inline bool
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
PkgDb::loadLockedFlake()
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
        nix::fmt( "database '%s' fingerprint '%s' does not match expected '%s'"
                , this->dbPath
                , fingerprintStr
                , this->fingerprint.to_string( nix::Base16, false )
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::writeInput()
{
  sqlite3pp::command cmd(
    this->db
  , "INSERT OR IGNORE INTO LockedFlake ( fingerprint, string, attrs ) VALUES"
    "  ( :fingerprint, :string, :attrs )"
  );
  std::string fpStr = fingerprint.to_string( nix::Base16, false );
  cmd.bind( ":fingerprint", fpStr, sqlite3pp::copy );
  cmd.bind( ":string", this->lockedRef.string, sqlite3pp::nocopy );
  cmd.bind( ":attrs",  this->lockedRef.attrs.dump(), sqlite3pp::copy );
  if ( sql_rc rc = cmd.execute(); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to write LockedFlaked info:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::initTables()
{
  if ( sql_rc rc = this->execute( sql_versions ); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to initialize DbVersions table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_input ); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to initialize LockedFlake table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_packageSets ); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to initialize AttrSets table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_packages ); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to initialize Packages table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  static const char * stmtVersions =
    "INSERT OR IGNORE INTO DbVersions ( name, version ) VALUES"
    "  ( 'pkgdb',        '" FLOX_PKGDB_VERSION        "' )"
    ", ( 'pkgdb_schema', '" FLOX_PKGDB_SCHEMA_VERSION "' )";

  if ( sql_rc rc = this->execute( stmtVersions ); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to write DbVersions info:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }
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
PkgDb::hasAttrSet( const flox::AttrPath & path )
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
PkgDb::getDescription( row_id descriptionId )
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
      throw PkgDbException( nix::fmt( "No such Descriptions.id %llu."
                                    , descriptionId
                                    )
                          );
    }
  return ( * i ).get<std::string>( 0 );
}


/* -------------------------------------------------------------------------- */

  bool
PkgDb::hasPackage( const flox::AttrPath & path )
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
PkgDb::getAttrSetId( const flox::AttrPath & path )
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
            nix::fmt( "No such AttrSet '%s'."
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
PkgDb::getAttrSetPath( row_id id )
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
          throw PkgDbException( nix::fmt( "No such `AttrSet.id' %llu.", id ) );
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
PkgDb::getPackageId( const flox::AttrPath & path )
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
        nix::fmt( "No such package %s.", nix::concatStringsSep( ".", path ) )
      );
    }
  return ( * i ).get<long long>( 0 );
}


/* -------------------------------------------------------------------------- */

  flox::AttrPath
PkgDb::getPackagePath( row_id id )
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
      throw PkgDbException( nix::fmt( "No such `Packages.id' %llu.", id ) );
    }
  flox::AttrPath path = this->getAttrSetPath( ( * i ).get<long long>( 0 ) );
  path.emplace_back( ( * i ).get<std::string>( 1 ) );
  return path;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetAttrSetId( const std::string & attrName, row_id parent )
{
  sqlite3pp::query qryId(
    this->db
  , "SELECT id FROM AttrSets "
    "WHERE ( attrName = :attrName ) AND ( parent = :parent )"
  );
  qryId.bind( ":attrName", attrName, sqlite3pp::copy );
  qryId.bind( ":parent", (long long) parent );
  auto i = qryId.begin();
  if ( i == qryId.end() )
    {
      sqlite3pp::command cmd(
        this->db
      , "INSERT OR IGNORE INTO AttrSets ( attrName, parent ) "
        "VALUES ( :attrName, :parent )"
      );
      cmd.bind( ":attrName", attrName, sqlite3pp::copy );
      cmd.bind( ":parent", (long long) parent );
      if ( sql_rc rc = cmd.execute(); isSQLError( rc ) )
        {
          throw PkgDbException(
            nix::fmt( "Failed to add AttrSet.id 'AttrSets[%ull].%s':(%d) %s"
                    , parent
                    , attrName
                    , rc
                    , this->db.error_msg()
                    )
          );
        }
      return this->db.last_insert_rowid();
    }
  return ( * i ).get<long long>( 0 );
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetAttrSetId( const flox::AttrPath & path )
{
  row_id id = 0;
  for ( const auto & p : path ) { id = addOrGetAttrSetId( p, id ); }
  return id;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetDescriptionId( const std::string & description )
{
  sqlite3pp::query qry(
    this->db
  , "SELECT id FROM Descriptions WHERE description = :description LIMIT 1"
  );
  qry.bind( ":description", description, sqlite3pp::copy );
  auto i = qry.begin();
  if ( i != qry.end() )
    {
      nix::Activity act(
        * nix::logger
      , nix::lvlDebug
      , nix::actUnknown
      , nix::fmt( "Found existing description in database: %s.", description )
      );
      return ( * i ).get<long long>( 0 );
    }

  sqlite3pp::command cmd(
    this->db
  , "INSERT INTO Descriptions ( description ) VALUES ( :description )"
  );
  cmd.bind( ":description", description, sqlite3pp::copy );
  nix::Activity act(
    * nix::logger
  , nix::lvlDebug
  , nix::actUnknown
  , nix::fmt( "Adding new description to database: %s.", description )
  );
  if ( sql_rc rc = cmd.execute(); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to add Description '%s':(%d) %s"
                , description
                , rc
                , this->db.error_msg()
                )
      );
    }
  return this->db.last_insert_rowid();
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addPackage( row_id           parentId
                 , std::string_view attrName
                 , flox::Cursor     cursor
                 , bool             replace
                 , bool             checkDrv
                 )
{
  #define _ADD_PKG_BODY                                                  \
   " INTO Packages ("                                                    \
   "  parentId, attrName, name, pname, version, semver, license"         \
   ", outputs, outputsToInstall, broken, unfree, descriptionId"          \
   ") VALUES ("                                                          \
   "  :parentId, :attrName, :name, :pname, :version, :semver, :license"  \
   ", :outputs, :outputsToInstall, :broken, :unfree, :descriptionId"     \
   ")"
  static const char * qryIgnore  = "INSERT OR IGNORE"  _ADD_PKG_BODY;
  static const char * qryReplace = "INSERT OR REPLACE" _ADD_PKG_BODY;

  sqlite3pp::command cmd( this->db, replace ? qryReplace : qryIgnore );

  /* We don't need to reference any `attrPath' related info here, so
   * we can avoid looking up the parent path by passing a phony one to the
   * `FlakePackage' constructor here. */
  FlakePackage pkg( cursor, { "packages", "x86_64-linux", "phony" }, checkDrv );
  std::string  attrNameS( attrName );

  cmd.bind( ":parentId", (long long) parentId );
  cmd.bind( ":attrName", attrNameS,     sqlite3pp::copy   );
  cmd.bind( ":name",     pkg._fullName, sqlite3pp::nocopy );
  cmd.bind( ":pname",    pkg._pname,    sqlite3pp::nocopy );

  if ( pkg._version.empty() )
    {
      cmd.bind( ":version" );  /* bind NULL */
    }
  else
    {
      cmd.bind( ":version", pkg._version, sqlite3pp::nocopy );
    }

  if ( pkg._semver.has_value() )
    {
      cmd.bind( ":semver", pkg._semver.value(), sqlite3pp::nocopy );
    }
  else
    {
      cmd.bind( ":semver" );  /* binds NULL */
    }

  {
    nlohmann::json j = pkg.getOutputs();
    cmd.bind( ":outputs", j.dump(), sqlite3pp::copy );
  }
  {
    nlohmann::json j = pkg.getOutputsToInstall();
    cmd.bind( ":outputsToInstall", j.dump(), sqlite3pp::copy );
  }


  if ( pkg._hasMetaAttr )
    {
      if ( auto m = pkg.getLicense(); m.has_value() )
        {
          cmd.bind( ":license", m.value(), sqlite3pp::copy );
        }
      else
        {
          cmd.bind( ":license" );
        }

      if ( auto m = pkg.isBroken(); m.has_value() )
        {
          cmd.bind( ":broken", (int) m.value() );
        }
      else
        {
          cmd.bind( ":broken" );
        }

      if ( auto m = pkg.isUnfree(); m.has_value() )
        {
          cmd.bind( ":unfree", (int) m.value() );
        }
      else /* TODO: Derive value from `license'? */
        {
          cmd.bind( ":unfree" );
        }

      if ( auto m = pkg.getDescription(); m.has_value() )
        {
          row_id descriptionId = this->addOrGetDescriptionId( m.value() );
          cmd.bind( ":descriptionId", (long long) descriptionId );
        }
      else
        {
          cmd.bind( ":descriptionId" );
        }
    }
  else
    {
      /* binds NULL */
      cmd.bind( ":license" );
      cmd.bind( ":broken" );
      cmd.bind( ":unfree" );
      cmd.bind( ":descriptionId" );
    }
  if ( sql_rc rc = cmd.execute(); isSQLError( rc ) )
    {
      throw PkgDbException(
        nix::fmt( "Failed to write Package '%s':(%d) %s"
                , pkg._fullName
                , rc
                , this->db.error_msg()
                )
      );
    }
  return this->db.last_insert_rowid();
}


/* -------------------------------------------------------------------------- */

  void
scrape(       flox::pkgdb::PkgDb & db
      ,       nix::SymbolTable   & syms
      , const flox::AttrPath     & prefix
      ,       flox::Cursor         cursor
      ,       Todos              & todo
      ,       bool                 transact
      )
{

  bool tryRecur = prefix[0] != "packages";

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "evaluating package set '%s'"
            , nix::concatStringsSep( ".", prefix )
            )
  );

  /* Lookup/create the `pathId' for for this attr-path in our DB.
   * This must be done before starting a transaction in the database
   * because it may need to read/write multiple times. */
  flox::pkgdb::row_id parentId = db.addOrGetAttrSetId( prefix );

  std::unique_ptr<sqlite3pp::transaction> txn;

  if ( transact )
    {
      /* Start a transaction */
      txn = std::make_unique<sqlite3pp::transaction>( db.db );
    }

  /* Scrape loop over attrs */
  for ( nix::Symbol & aname : cursor->getAttrs() )
    {
      const std::string pathS =
        nix::concatStringsSep( ".", prefix ) + "." + syms[aname];

      if ( syms[aname] == "recurseForDerivations" ) { continue; }

      nix::Activity act(
        * nix::logger
      , nix::lvlTalkative
      , nix::actUnknown
      , nix::fmt( "\tevaluating attribute '%s'", pathS )
      );

      try
        {
          flox::Cursor child = cursor->getAttr( aname );
          if ( child->isDerivation() )
            {
              db.addPackage( parentId, syms[aname], child );
              continue;
            }
          if ( ! tryRecur ) { continue; }
          if ( auto m = child->maybeGetAttr( "recurseForDerivations" );
                    ( m != nullptr ) && m->getBool()
                  )
            {
              flox::AttrPath path = prefix;
              path.emplace_back( syms[aname] );
              nix::logger->log( nix::lvlTalkative
                              , nix::fmt( "\tpushing target '%s'", pathS )
                              );
              todo.push( std::make_pair( std::move( path ), child ) );
            }
        }
      catch( const nix::EvalError & e )
        {
          /* Ignore errors in `legacyPackages' and `catalog' */
          if ( tryRecur )
            {
              /* Only print eval errors in "debug" mode. */
              nix::ignoreException( nix::lvlDebug );
            }
          else
            {
              if ( transact )
                {
                  txn->rollback();  /* Revert transaction changes */
                }
              throw e;
            }
        }
    }

  if ( transact )
    {
      txn->commit();  /* Commit transaction changes */
    }
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
