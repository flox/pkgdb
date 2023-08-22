/* ========================================================================== *
 *
 * @file flox/pkgdb/read.hh
 *
 * @brief Interfaces for reading a SQLite3 package set database.
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

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/command.hh"


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


/* -------------------------------------------------------------------------- */

struct PkgDbException : public FloxException {
  std::filesystem::path dbPath;
  PkgDbException( const std::filesystem::path & dbPath
                ,       std::string_view        msg
                )
    : FloxException( msg ), dbPath( dbPath )
  {}
};  /* End struct `PkgDbException' */


/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
std::string genPkgDbName( const Fingerprint & fingerprint );


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDbReadOnly {

/* -------------------------------------------------------------------------- */

  /* Data */

  public:

    SQLiteDb    db;           /**< SQLite3 database handle.         */
    Fingerprint fingerprint;  /**< Unique hash of associated flake. */

    std::filesystem::path dbPath;  /**< Absolute path to database. */

    /** Locked _flake reference_ for database's flake. */
    struct LockedFlakeRef {
      std::string    string;  /**< Locked URI string.  */
      nlohmann::json attrs;   /**< Exploded form of URI as an attr-set. */
    } lockedRef;


/* -------------------------------------------------------------------------- */

  /* Errors */

  public:

    struct NoSuchDatabase : PkgDbException {
      NoSuchDatabase( const PkgDbReadOnly & db )
        : PkgDbException(
            db.dbPath
          , std::string( "No such database '" + db.dbPath.string() + "'." )
          )
      {}
    };  /* End struct `NoSuchDatabase' */


/* -------------------------------------------------------------------------- */

  /* Internal Helpers */

  protected:

    /** Set @a this `PkgDb` `lockedRef` fields from database metadata. */
    void loadLockedFlake();


/* -------------------------------------------------------------------------- */

  /* Constructors */

  protected:

    /**
     * Dummy constructor required for child classes so that they can open
     * databases in read-only mode.
     * Does NOT attempt to create a database if one does not exist.
     */
    PkgDbReadOnly() : db(), fingerprint( nix::htSHA256 ), dbPath() {}


  public:

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     * @param dbPath Absolute path to database file.
     */
    PkgDbReadOnly( const Fingerprint      & fingerprint
                 ,       std::string_view   dbPath
                 )
      : db( dbPath, SQLITE_OPEN_READONLY )
      , fingerprint( fingerprint )
      , dbPath( dbPath )
    {
      this->loadLockedFlake();
    }

    /**
     * Opens an existing database.
     * Does NOT attempt to create a database if one does not exist.
     * @param dbPath Absolute path to database file.
     */
    PkgDbReadOnly( std::string_view dbPath )
      : db( dbPath, SQLITE_OPEN_READONLY )
      , fingerprint( nix::htSHA256 )
      , dbPath( dbPath )
    {
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          throw NoSuchDatabase( * this );
        }
      this->loadLockedFlake();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     */
    PkgDbReadOnly( const Fingerprint & fingerprint )
      : PkgDbReadOnly( fingerprint, genPkgDbName( fingerprint ) )
    {}


/* -------------------------------------------------------------------------- */

  /* Queries */

  public:

    /** @return The Package Database schema version. */
    std::string getDbVersion();

    /**
     * Check to see if database has packages under the attribute path
     * prefix @a path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return `true` iff the database has an `AttrSet` at @a path.
     */
    bool hasAttrSet( const flox::AttrPath & path );

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

    /**
     * Enumerate all `AttrSets.id`s which are descendants ( recursively ) of
     * the `AttrSets.id` @a root.
     * @param root The `AttrSets.id` to target.
     * @return All `AttrSets.id`s which are descendants of @a root.
     */
    std::vector<row_id> getDescendantAttrSets( row_id root );


/* -------------------------------------------------------------------------- */

};  /* End class `PkgDbReadOnly' */


/* -------------------------------------------------------------------------- */

bool isSQLError( int rc );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
