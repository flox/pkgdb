/* ========================================================================== *
 *
 * @file pkg-db.hh
 *
 * @brief Interfaces for operating on a SQLite3 package set database.
 *
 * TODO: Description field
 * TODO: Optimize filtering by `parent' by reading `pathId's into a
 *       `std::map<long, std::vector<std::string>>'.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include <sqlite3pp.hh>

#include "flox/raw-package.hh"


/* -------------------------------------------------------------------------- */

#define FLOX_PKGDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces for caching package metadata in SQLite3 databases. */
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

using Fingerprint = nix::flake::Fingerprint;
using SQLiteDb    = sqlite3pp::database;
using AttrPathV   = std::vector<std::string_view>;
using AttrPath    = std::vector<std::string>;
using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using row_id      = uint64_t;


/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
std::string getPkgDbName( const Fingerprint & fingerprint );

/** Get an absolute path to the `PkgDb' for a locked flake. */
  static inline std::string
getPkgDbName( const nix::flake::LockedFlake & flake )
{
  return getPkgDbName( flake.getFingerprint() );
}


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDb {

  /* Data */

  private:
    /** Create tables in database if they do not exist. */
    void initTables();

    /** Set @a this `PkgDb` `lockedRef` fields from database metadata. */
    void loadLockedRef();


  public:

          SQLiteDb    db;           /**< SQLite3 database handle.         */
    const Fingerprint fingerprint;  /**< Unique hash of associated flake. */

    /** Locked _flake reference_ for database's flake. */
    const struct {
      std::string    string;  /**< Locked URI string.  */
      nlohmann::json attrs;   /**< Exploded form of URI as an attr-set. */
    } lockedRef;


/* -------------------------------------------------------------------------- */

  /* Constructors */

  public:
    PkgDb() : fingerprint( nix::htSHA256 ) {}

    /** Opens a DB associated with a locked flake. */
    PkgDb( const nix::flake::LockedFlake & flake );

    /** Opens a DB directly by its fingerprint hash. */
    PkgDb( const Fingerprint & fingerprint );


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
     int execute( std::string_view stmt );


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
    bool hasPackageSet( const AttrPathV & path );

    /**
     * Get the `PackageSet.pathId` for a given path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id getPackageSetId( const AttrPathV & path );


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
    bool hasPackage( const AttrPathV & path );


/* -------------------------------------------------------------------------- */

  /* Insert */

  public:

    /**
     * Get the `PackageSet.pathId` for a given path if it exists, or insert a
     * new row for @a path and return its `pathId`.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id addOrGetPackageSetId( const AttrPathV & path );

    /**
     * Get the `Descriptions.id` for a given string if it exists, or insert a
     * new row for @a description and return its `id`.
     * @param description A string describing a package.
     * @return A unique `row_id` ( unsigned 64bit int ) associated
     *         with @a description.
     */
    row_id addOrGetDescriptionId( std::string_view description );

    /**
     * Adds a package to the database.
     * @param parentId The `pathId` associated with the parent path.
     * @param attrName The name of the attribute name to be added ( last element
     *                 of the attribute path ).
     * @param cursor An attribute cursor to scrape data from.
     * @param replace Whether to replace/ignore existing rows.
     * @return The `Packages.id` value for the added package.
     */
    row_id addPackage( row_id           parentId
                     , std::string_view attrName
                     , Cursor           cursor
                     , bool             replace = false
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
