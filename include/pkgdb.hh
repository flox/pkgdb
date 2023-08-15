/* ========================================================================== *
 *
 * @file pkgdb.hh
 *
 * @brief Interfaces for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>
#include <queue>
#include <filesystem>
#include <functional>

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include <sqlite3pp.hh>

#include "flox/exceptions.hh"
#include "flox/types.hh"
#include "flox/command.hh"


/* -------------------------------------------------------------------------- */

/* This is passed in by `make' and is set by `<pkgdb>/version' */
#ifndef FLOX_PKGDB_VERSION
#  define FLOX_PKGDB_VERSION  "NO.VERSION"
#endif
#define FLOX_PKGDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces for caching package metadata in SQLite3 databases. */
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

/** A unique hash associated with a locked flake. */
using Fingerprint = nix::flake::Fingerprint;
using SQLiteDb    = sqlite3pp::database;
using row_id      = uint64_t;  /**< A _row_ index in a SQLite3 table. */
using sql_rc      = int;       /**< `SQLITE_*` result code. */

using Target = std::pair<flox::AttrPath, flox::Cursor>;
using Todos  = std::queue<Target, std::list<Target>>;


/* -------------------------------------------------------------------------- */

struct PkgDbException : public FloxException {
  PkgDbException( std::string_view msg ) : FloxException( msg ) {}
};  /* End struct `PkgDbException' */


/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
std::string genPkgDbName( const Fingerprint & fingerprint );

/** Get an absolute path to the `PkgDb' for a locked flake. */
  static inline std::string
genPkgDbName( const nix::flake::LockedFlake & flake )
{
  return genPkgDbName( flake.getFingerprint() );
}


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDb {

/* -------------------------------------------------------------------------- */

  /* Data */

  public:

          SQLiteDb    db;           /**< SQLite3 database handle.         */
          Fingerprint fingerprint;  /**< Unique hash of associated flake. */

    const std::filesystem::path dbPath;  /**< Absolute path to database. */

    /** Locked _flake reference_ for database's flake. */
    struct LockedFlakeRef {
      std::string    string;  /**< Locked URI string.  */
      nlohmann::json attrs;   /**< Exploded form of URI as an attr-set. */
    } lockedRef;


/* -------------------------------------------------------------------------- */

  /* Internal Helpers */

  private:

    /** Create tables in database if they do not exist. */
    void initTables();

    /** Set @a this `PkgDb` `lockedRef` fields from database metadata. */
    void loadLockedFlake();

    /**
     * Write @a this `PkgDb` `lockedRef` and `fingerprint` fields to
     * database metadata.
     */
    void writeInput();


/* -------------------------------------------------------------------------- */

  /* Constructors */

  public:
    /**
     * Opens an existing database.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( std::string_view dbPath )
      : db( dbPath ), fingerprint( nix::htSHA256 ), dbPath( dbPath )
    {
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          throw PkgDbException( nix::fmt( "No such database '%s'.", dbPath ) );
        }
      this->initTables();
      this->loadLockedFlake();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * @param fingerprint Unique hash associated with locked flake.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( const Fingerprint      & fingerprint
         ,       std::string_view   dbPath
         )
      : db( dbPath ), fingerprint( fingerprint ), dbPath( dbPath )
    {
      this->initTables();
      this->loadLockedFlake();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * @param fingerprint Unique hash associated with locked flake.
     */
    PkgDb( const Fingerprint & fingerprint )
      : PkgDb( fingerprint, genPkgDbName( fingerprint ) )
    {}

    /**
     * Opens a DB associated with a locked flake.
     * @param flake Flake associated with the db. Used to write input metadata.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( const nix::flake::LockedFlake & flake
         ,       std::string_view          dbPath
         )
      : db( dbPath )
      , fingerprint( flake.getFingerprint() )
      , dbPath( dbPath )
      , lockedRef( {
          flake.flake.lockedRef.to_string()
        , nix::fetchers::attrsToJSON( flake.flake.lockedRef.toAttrs() )
        } )
    {
      initTables();
      writeInput();
    }

    /**
     * Opens a DB associated with a locked flake.
     * @param flake Flake associated with the db. Used to write input metadata.
     */
    PkgDb( const nix::flake::LockedFlake & flake )
      : PkgDb( flake, genPkgDbName( flake.getFingerprint() ) )
    {}


/* -------------------------------------------------------------------------- */

  /* Basic Operations */

  public:

    /** @return The Package Database schema version. */
    std::string getDbVersion();

    /**
     * Execute a raw sqlite statement on the database.
     * @param stmt String statement to execute.
     * @return `SQLITE_*` [error code](https://www.sqlite.org/rescode.html).
     */
       inline sql_rc
     execute( const char * stmt )
     {
       sqlite3pp::command cmd( this->db, stmt );
       return cmd.execute();
     }

    /**
     * Execute raw sqlite statements on the database.
     * @param stmt String statement to execute.
     * @return `SQLITE_*` [error code](https://www.sqlite.org/rescode.html).
     */
       inline sql_rc
     execute_all( const char * stmt )
     {
       sqlite3pp::command cmd( this->db, stmt );
       return cmd.execute_all();
     }


/* -------------------------------------------------------------------------- */

  /* Queries */

  public:

    /**
     * Check to see if database has packages under the attribute path
     * prefix @a path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return `true` iff the database has one or more rows in the `Packages`
     *         table with `path` as the _parent_.
     */
    bool hasPackageSet( const flox::AttrPath & path );

    /**
     * Get the `AttrSet.id` for a given path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id getAttrSetId( const flox::AttrPath & path );

    /**
     * Get the attribute path for a given `AttrSet.id`.
     * @param id A unique `row_id` ( unsigned 64bit int ).
     * @return An attribute path prefix such as `packages.x86_64-linux` or
     *         `legacyPackages.aarch64-darwin.python3Packages`.
     */
    flox::AttrPath getAttrSetPath( row_id id );

    /**
     * Get the `Packages.id` for a given path.
     * @param path An attribute path prefix such as
     *             `packages.x86_64-linux.hello` or
     *             `legacyPackages.aarch64-darwin.python3Packages.pip`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id getPackageId( const flox::AttrPath & path );

    /**
     * Get the attribute path for a given `Packages.id`.
     * @param id A unique `row_id` ( unsigned 64bit int ).
     * @return An attribute path such as `packages.x86_64-linux.hello` or
     *         `legacyPackages.aarch64-darwin.python3Packages.pip`.
     */
    flox::AttrPath getPackagePath( row_id id );

    /**
     * Check to see if database has a package at the attribute path @a path.
     * @param path An attribute path such as `packages.x86_64-linux.hello` or
     *             `legacyPackages.aarch64-darwin.python3Packages.pip`.
     * @return `true` iff the database has a rows in the `Packages`
     *         table with `path` as the _absolute path_.
     */
    bool hasPackage( const flox::AttrPath & path );


    /**
     * Get the `Description.description` for a given `Description.id`.
     * @param descriptionId The row id to lookup.
     * @return A string describing a package.
     */
    std::string getDescription( row_id descriptionId );


/* -------------------------------------------------------------------------- */

  /* Insert */

  public:

    /**
     * Get the `AttrSet.id` for a given child of the attribute set associated
     * with `parent` if it exists, or insert a new row for @a path and return
     * its `id`.
     * @param attrName An attribute set field name.
     * @param parent The `AttrSet.id` containing @a attrName.
     *               The `id` 0 may be used to indicate that @a attrName has no
     *               parent attribute set.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with
     *         @a attrName under @a parent.
     */
    row_id addOrGetAttrSetId( const std::string & attrName, row_id parent = 0 );

    /**
     * Get the `AttrSet.id` for a given path if it exists, or insert a
     * new row for @a path and return its `pathId`.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id addOrGetAttrSetId( const flox::AttrPath & path );

    /**
     * Get the `Descriptions.id` for a given string if it exists, or insert a
     * new row for @a description and return its `id`.
     * @param description A string describing a package.
     * @return A unique `row_id` ( unsigned 64bit int ) associated
     *         with @a description.
     */
    row_id addOrGetDescriptionId( const std::string & description );

    /**
     * Adds a package to the database.
     * @param parentId The `pathId` associated with the parent path.
     * @param attrName The name of the attribute name to be added ( last element
     *                 of the attribute path ).
     * @param cursor An attribute cursor to scrape data from.
     * @param replace Whether to replace/ignore existing rows.
     * @param checkDrv Whether to check `isDerivation` for @a cursor.
     *                 Skipping this check is a slight optimization for cases
     *                 where the caller has already checked themselves.
     * @return The `Packages.id` value for the added package.
     */
    row_id addPackage( row_id           parentId
                     , std::string_view attrName
                     , flox::Cursor     cursor
                     , bool             replace  = false
                     , bool             checkDrv = true
                     );


/* -------------------------------------------------------------------------- */

};  /* End class `PkgDb' */


/* -------------------------------------------------------------------------- */

/**
 * Scrape package definitions from an attribute set, adding any attributes
 * marked with `recurseForDerivatsions = true` to @a todo list.
 * @param db     Database to write to.
 * @param syms   Symbol table from @a cursor evaluator.
 * @param prefix Attribute path to scrape.
 * @param cursor `nix` evaluator cursor associated with @a prefix
 * @param todo Queue to add `recurseForDerivations = true` cursors to so they
 *             may be scraped by later invocations.
 */
  void
scrape(       flox::pkgdb::PkgDb & db
      ,       nix::SymbolTable   & syms
      , const flox::AttrPath     & prefix
      ,       flox::Cursor         cursor
      ,       Todos              & todo
      );




/* -------------------------------------------------------------------------- */

/** Scrape a flake prefix producing a SQLite3 database with package metadata. */
struct ScrapeCommand
  : public command::PkgDbMixin
  , public command::AttrPathMixin
{
  command::VerboseParser parser;

  bool force = false;  /**< Whether to force re-evaluation. */

  ScrapeCommand();

  /** Invoke "child" `preProcessArgs` for `PkgDbMixin` and `AttrPathMixin`. */
  void postProcessArgs() override;

  /**
   * Execute the `scrape` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `ScrapeCommand' */


/* -------------------------------------------------------------------------- */

/**
 * Minimal set of DB queries, largely focused on looking up info that is
 * non-trivial to query with a "plain" SQLite statement.
 * This subcommand has additional subcommands:
 * - `pkgdb get id [--pkg] DB-PATH ATTR-PATH...`
 *   + Lookup `(AttrSet|Packages).id` for `ATTR-PATH`.
 * - `pkgdb get path [--pkg] DB-PATH ID`
 *   + Lookup `AttrPath` for `(AttrSet|Packages).id`.
 * - `pkgdb get flake DB-PATH`
 *   + Dump the `LockedFlake` table including fingerprint, locked-ref, etc.
 * - `pkgdb get db FLAKE-REF`
 *   + Print the absolute path to the associated flake's db.
 */
struct GetCommand
  : public command::PkgDbMixin
  , public command::AttrPathMixin
{
  command::VerboseParser parser;  /**< `get`       parser */
  command::VerboseParser pId;     /**< `get id`    parser  */
  command::VerboseParser pPath;   /**< `get path`  parser */
  command::VerboseParser pFlake;  /**< `get flake` parser */
  command::VerboseParser pDb;     /**< `get db`    parser */
  bool                   isPkg  = false;
  flox::pkgdb::row_id    id     = 0;

  GetCommand();

  /** Prevent "child" `preProcessArgs` routines from running */
  void postProcessArgs() override {}

  /**
   * Execute the `get id` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runId();

  /**
   * Execute the `get path` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runPath();

  /**
   * Execute the `get flake` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runFlake();

  /**
   * Execute the `get db` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int runDb();

  /**
   * Execute the `get` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int run();

};  /* End struct `GetCommand' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
