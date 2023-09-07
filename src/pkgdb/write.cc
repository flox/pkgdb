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

namespace flox::pkgdb {

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
  if ( sql_rc rcode = cmd.execute(); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to write LockedFlaked info:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::initTables()
{
  if ( sql_rc rcode = this->execute( sql_versions ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize DbVersions table:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rcode = this->execute_all( sql_input ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize LockedFlake table:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rcode = this->execute_all( sql_attrSets ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize AttrSets table:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rcode = this->execute_all( sql_packages ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize Packages table:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }

  if ( sql_rc rcode = this->execute_all( sql_views ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to initialize views:(%d) %s"
                , rcode
                , this->db.error_msg()
                )
      );
    }

  static const char * stmtVersions =
    "INSERT OR IGNORE INTO DbVersions ( name, version ) VALUES"
    "  ( 'pkgdb',        '" FLOX_PKGDB_VERSION        "' )"
    ", ( 'pkgdb_schema', '" FLOX_PKGDB_SCHEMA_VERSION "' )";

  if ( sql_rc rcode = this->execute( stmtVersions ); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to write DbVersions info:(%d) %s"
                , rcode
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
  qryId.bind( ":parent", static_cast<long long>( parent ) );
  auto row = qryId.begin();
  if ( row == qryId.end() )
    {
      sqlite3pp::command cmd(
        this->db
      , "INSERT OR IGNORE INTO AttrSets ( attrName, parent ) "
        "VALUES ( :attrName, :parent )"
      );
      cmd.bind( ":attrName", attrName, sqlite3pp::copy );
      cmd.bind( ":parent", static_cast<long long>( parent ) );
      if ( sql_rc rcode = cmd.execute(); isSQLError( rcode ) )
        {
          throw PkgDbException(
            this->dbPath
          , nix::fmt( "Failed to add AttrSet.id 'AttrSets[%ull].%s':(%d) %s"
                    , parent
                    , attrName
                    , rcode
                    , this->db.error_msg()
                    )
          );
        }
      return this->db.last_insert_rowid();
    }
  return ( * row ).get<long long>( 0 );
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetAttrSetId( const flox::AttrPath & path )
{
  row_id row = 0;
  for ( const auto & attr : path ) { row = addOrGetAttrSetId( attr, row ); }
  return row;
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
  auto rows = qry.begin();
  if ( rows != qry.end() )
    {
      nix::Activity act(
        * nix::logger
      , nix::lvlDebug
      , nix::actUnknown
      , nix::fmt( "Found existing description in database: %s.", description )
      );
      return ( * rows ).get<long long>( 0 );
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
  if ( sql_rc rcode = cmd.execute(); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to add Description '%s':(%d) %s"
                , description
                , rcode
                , this->db.error_msg()
                )
      );
    }
  return this->db.last_insert_rowid();
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addPackage(       row_id             parentId
                 ,       std::string_view   attrName
                 , const flox::Cursor     & cursor
                 ,       bool               replace
                 ,       bool               checkDrv
                 )
{
  // NOLINTNEXTLINE
  #define ADD_PKG_BODY                                                   \
   " INTO Packages ("                                                    \
   "  parentId, attrName, name, pname, version, semver, license"         \
   ", outputs, outputsToInstall, broken, unfree, descriptionId"          \
   ") VALUES ("                                                          \
   "  :parentId, :attrName, :name, :pname, :version, :semver, :license"  \
   ", :outputs, :outputsToInstall, :broken, :unfree, :descriptionId"     \
   ")"
  static const char * qryIgnore  = "INSERT OR IGNORE"  ADD_PKG_BODY;
  static const char * qryReplace = "INSERT OR REPLACE" ADD_PKG_BODY;

  sqlite3pp::command cmd( this->db, replace ? qryReplace : qryIgnore );

  /* We don't need to reference any `attrPath' related info here, so
   * we can avoid looking up the parent path by passing a phony one to the
   * `FlakePackage' constructor here. */
  FlakePackage pkg( cursor, { "packages", "x86_64-linux", "phony" }, checkDrv );
  std::string  attrNameS( attrName );

  cmd.bind( ":parentId", static_cast<long long>( parentId ) );
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
      cmd.bind( ":semver", * pkg._semver, sqlite3pp::nocopy );
    }
  else
    {
      cmd.bind( ":semver" );  /* binds NULL */
    }

  {
    nlohmann::json jOutputs = pkg.getOutputs();
    cmd.bind( ":outputs", jOutputs.dump(), sqlite3pp::copy );
  }
  {
    nlohmann::json jOutsInstall = pkg.getOutputsToInstall();
    cmd.bind( ":outputsToInstall", jOutsInstall.dump(), sqlite3pp::copy );
  }


  if ( pkg._hasMetaAttr )
    {
      if ( auto maybe = pkg.getLicense(); maybe.has_value() )
        {
          cmd.bind( ":license", * maybe, sqlite3pp::copy );
        }
      else
        {
          cmd.bind( ":license" );
        }

      if ( auto maybe = pkg.isBroken(); maybe.has_value() )
        {
          cmd.bind( ":broken", static_cast<int>( * maybe ) );
        }
      else
        {
          cmd.bind( ":broken" );
        }

      if ( auto maybe = pkg.isUnfree(); maybe.has_value() )
        {
          cmd.bind( ":unfree", static_cast<int>( * maybe ) );
        }
      else /* TODO: Derive value from `license'? */
        {
          cmd.bind( ":unfree" );
        }

      if ( auto maybe = pkg.getDescription(); maybe.has_value() )
        {
          row_id descriptionId = this->addOrGetDescriptionId( * maybe );
          cmd.bind( ":descriptionId", static_cast<long long>( descriptionId ) );
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
  if ( sql_rc rcode = cmd.execute(); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to write Package '%s':(%d) %s"
                , pkg._fullName
                , rcode
                , this->db.error_msg()
                )
      );
    }
  return this->db.last_insert_rowid();
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::setPrefixDone( const flox::AttrPath & prefix, bool done )
{
  row_id prefixId = this->addOrGetAttrSetId( prefix );
  sqlite3pp::command cmd( this->db, R"SQL(
    UPDATE AttrSets SET done = :done WHERE id in (
      WITH RECURSIVE Tree AS (
        SELECT id, parent, 0 as depth FROM AttrSets
        WHERE ( id = :root )
        UNION ALL SELECT O.id, O.parent, ( Parent.depth + 1 ) AS depth
        FROM AttrSets O
        JOIN Tree AS Parent ON ( Parent.id = O.parent )
      ) SELECT C.id FROM Tree AS C
      JOIN AttrSets AS Parent ON ( C.parent = Parent.id )
    )
  )SQL" );
  cmd.bind( ":root", static_cast<long long>( prefixId ) );
  cmd.bind( ":done", static_cast<int>( done ) );
  if ( sql_rc rcode = cmd.execute(); isSQLError( rcode ) )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Failed to set AttrSets.done for subtree '%s':(%d) %s"
                , nix::concatStringsSep( ".", prefix )
                , rcode
                , this->db.error_msg()
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

/* NOTE:
 * Benchmarks on large catalogs have indicated that using a _todo_ queue instead
 * of recursion is faster and consumes less memory.
 * Repeated runs against `nixpkgs-flox` come in at ~2m03s using recursion and
 * ~1m40s using a queue. */
  void
PkgDb::scrape(       nix::SymbolTable & syms
             , const Target           & target
             ,       Todos            & todo
             )
{
  const auto & [prefix, cursor, parentId] = target;

  /* If it has previously been scraped then bail out. */
  if ( this->completedAttrSet( parentId ) ) { return; }

  bool tryRecur = prefix.front() != "packages";

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "evaluating package set '%s'"
            , nix::concatStringsSep( ".", prefix )
            )
  );

  /* Scrape loop over attrs */
  for ( nix::Symbol & aname : cursor->getAttrs() )
    {
      if ( syms[aname] == "recurseForDerivations" ) { continue; }

      /* Used for logging, but can skip it at low verbosity levels. */
      const std::string pathS =
        ( nix::lvlTalkative <= nix::verbosity )
        ? nix::concatStringsSep( ".", prefix ) + "." + syms[aname]
        : "";

      nix::Activity act(
        * nix::logger
      , nix::lvlTalkative
      , nix::actUnknown
      , "\tevaluating attribute '" + pathS + "'"
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
          if ( auto maybe = child->maybeGetAttr( "recurseForDerivations" );
                    ( maybe != nullptr ) && maybe->getBool()
                  )
            {
              flox::AttrPath path = prefix;
              path.emplace_back( syms[aname] );
              if ( nix::lvlTalkative <= nix::verbosity )
                {
                  nix::logger->log(
                    nix::lvlTalkative
                  , "\tpushing target '" + pathS + "'"
                  );
                }
              row_id childId = this->addOrGetAttrSetId( syms[aname], parentId );
              todo.emplace( std::make_tuple(
                              std::move( path )
                            , std::move( child )
                            , childId
                            )
                          );
            }
        }
      catch( const nix::EvalError & err )
        {
          /* Ignore errors in `legacyPackages' and `catalog' */
          if ( tryRecur )
            {
              /* Only print eval errors in "debug" mode. */
              nix::ignoreException( nix::lvlDebug );
            }
          else
            {
              throw;
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
