/* ========================================================================== *
 *
 * @file pkg-db.hh
 *
 * @brief Interfaces for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include <sqlite3pp.hh>

#include "flox/exceptions.hh"


/* -------------------------------------------------------------------------- */

// TODO: Create a `config.h' for these
#define FLOX_PKGDB_VERSION         "0.1.0"
#define FLOX_PKGDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces for caching package metadata in SQLite3 databases. */
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

using Fingerprint = nix::flake::Fingerprint;
using SQLiteDb    = sqlite3pp::database;
using AttrPath    = std::vector<std::string>;
using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using row_id      = uint64_t;
using sql_rc      = int;       /**< `SQLITE_*` result code. */


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
    const std::string dbPath;       /**< Absolute path to database.       */

    /** Locked _flake reference_ for database's flake. */
    struct {
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
      : dbPath( dbPath ), db( dbPath ), fingerprint( nix::htSHA256 )
    {
      if ( ! std::filesystem::exists( dbPath ) )
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
      : fingerprint( fingerprint ), dbPath( dbPath ), db( dbPath )
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
      : fingerprint( flake.getFingerprint() )
      , db( dbPath )
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
     * @return `true` iif the database has one or more rows in the `Packages`
     *         table with `path` as the _parent_.
     */
    bool hasPackageSet( const AttrPath & path );

    /**
     * Get the `AttrSet.id` for a given path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id getPackageSetId( const AttrPath & path );

    /**
     * Get the attribute path for a given `AttrSet.id`.
     * @param id A unique `row_id` ( unsigned 64bit int ).
     * @return An attribute path prefix such as `packages.x86_64-linux` or
     *         `legacyPackages.aarch64-darwin.python3Packages`.
     */
    AttrPath getPackageSetPath( row_id id );


    /**
     * Get the `Description.description` for a given `Description.id`.
     * @param descriptionId The row id to lookup.
     * @return A string describing a package.
     */
    std::string getDescription( row_id descriptionId );


    /**
     * Check to see if database has a package at the attribute path @a path.
     * @param path An attribute path such as `packages.x86_64-linux.hello` or
     *             `legacyPackages.aarch64-darwin.python3Packages.pip`.
     * @return `true` iif the database has a rows in the `Packages`
     *         table with `path` as the _absolute path_.
     */
    bool hasPackage( const AttrPath & path );


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
    row_id addOrGetPackageSetId( const AttrPath & path );

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
                     , Cursor           cursor
                     , bool             replace  = false
                     , bool             checkDrv = true
                     );


/* -------------------------------------------------------------------------- */

};  /* End class `PkgDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
