/* ========================================================================== *
 *
 * @file pkgdb/write.cc
 *
 * @brief Implementations for writing to a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <memory>

#include "flox/pkgdb/write.hh"
#include "flox/flake-package.hh"

#include "./schemas.hh"



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
        this->dbPath
      , nix::fmt( "Failed to write LockedFlaked info:(%d) %s"
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
        this->dbPath
      , nix::fmt( "Failed to initialize DbVersions table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_input ); isSQLError( rc ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize LockedFlake table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_attrSets ); isSQLError( rc ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize AttrSets table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_packages ); isSQLError( rc ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize Packages table:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rc = this->execute_all( sql_views ); isSQLError( rc ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize views:(%d) %s"
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
        this->dbPath
      , nix::fmt( "Failed to write DbVersions info:(%d) %s"
                , rc
                , this->db.error_msg()
                )
      );
    }
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
            this->dbPath
          , nix::fmt( "Failed to add AttrSet.id 'AttrSets[%ull].%s':(%d) %s"
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
        this->dbPath
      , nix::fmt( "Failed to add Description '%s':(%d) %s"
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
        this->dbPath
      , nix::fmt( "Failed to write Package '%s':(%d) %s"
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
PkgDb::scrape(       nix::SymbolTable & syms
             , const flox::AttrPath   & prefix
             ,       flox::Cursor       cursor
             ,       Todos            & todo
             )
{

  bool tryRecur = prefix.front() != "packages";

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "evaluating package set '%s'"
            , nix::concatStringsSep( ".", prefix )
            )
  );

  /* Lookup/create the `pathId' for for this attr-path in our DB. */
  flox::pkgdb::row_id parentId = this->addOrGetAttrSetId( prefix );

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
              this->addPackage( parentId, syms[aname], child );
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
              throw e;
            }
        }
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
