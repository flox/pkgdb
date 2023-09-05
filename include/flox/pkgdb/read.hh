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
#include "flox/package.hh"
#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

/* This is passed in by `make' and is set by `<pkgdb>/version' */
#ifndef FLOX_PKGDB_VERSION
#  define FLOX_PKGDB_VERSION  "NO.VERSION"
#endif
#define FLOX_PKGDB_SCHEMA_VERSION  "1.0.0"


/* -------------------------------------------------------------------------- */

/** Interfaces for caching package metadata in SQLite3 databases. */
namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/** A unique hash associated with a locked flake. */
using Fingerprint = nix::flake::Fingerprint;
using SQLiteDb    = sqlite3pp::database;
using sql_rc      = int;                      /**< `SQLITE_*` result code. */


/* -------------------------------------------------------------------------- */

struct PkgDbException : public FloxException {
  std::filesystem::path dbPath;
  PkgDbException( std::filesystem::path dbPath, std::string_view msg )
    : FloxException( msg ), dbPath( std::move( dbPath ) )
  {}
};  /* End struct `PkgDbException' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Get the default pkgdb cache directory to save databases.
 *
 * The environment variable `PKGDB_CACHEDIR` is respected if it is set,
 * otherwise we use
 * `${XDG_CACHE_HOME:-$HOME/.cache}/flox/pkgdb-v<SCHEMA-MAJOR>`.
 */
std::filesystem::path getPkgDbCachedir();

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
std::filesystem::path genPkgDbName(
  const Fingerprint           & fingerprint
, const std::filesystem::path & cacheDir = getPkgDbCachedir()
);


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDbReadOnly {

/* -------------------------------------------------------------------------- */

  /* Data */

  public:

    Fingerprint           fingerprint;  /**< Unique hash of associated flake. */
    std::filesystem::path dbPath;       /**< Absolute path to database. */
    SQLiteDb              db;           /**< SQLite3 database handle. */

    /** Locked _flake reference_ for database's flake. */
    struct LockedFlakeRef {
      std::string string;  /**< Locked URI string.  */
      /** Exploded form of URI as an attr-set. */
      nlohmann::json attrs = nlohmann::json::object();
    } lockedRef;


/* -------------------------------------------------------------------------- */

  /* Errors */

  // public:

    struct NoSuchDatabase : PkgDbException {
      explicit NoSuchDatabase( const PkgDbReadOnly & pdb )
        : PkgDbException(
            pdb.dbPath
          , std::string( "No such database '" + pdb.dbPath.string() + "'." )
          )
      {}
    };  /* End struct `NoSuchDatabase' */


/* -------------------------------------------------------------------------- */

  /* Internal Helpers */

  protected:

    /** Set @a this `PkgDb` `lockedRef` fields from database metadata. */
    void loadLockedFlake();


  private:

    /**
     * Open SQLite3 db connection at @a dbPath.
     * Throw an error if no database exists.
     */
    void init();


/* -------------------------------------------------------------------------- */

  /* Constructors */

  protected:

    /**
     * Dummy constructor required for child classes so that they can open
     * databases in read-only mode.
     * Does NOT attempt to create a database if one does not exist.
     */
    PkgDbReadOnly() : fingerprint( nix::htSHA256 ) {}


  public:

    /**
     * Opens an existing database.
     * Does NOT attempt to create a database if one does not exist.
     * @param dbPath Absolute path to database file.
     */
    explicit PkgDbReadOnly( std::string_view dbPath )
      : fingerprint( nix::htSHA256 )  /* Filled by `loadLockedFlake' later */
      , dbPath( dbPath )
    {
      this->init();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     * @param dbPath Absolute path to database file.
     */
    PkgDbReadOnly( const Fingerprint      & fingerprint
                 ,       std::string_view   dbPath
                 )
      : fingerprint( fingerprint )
      , dbPath( dbPath )
    {
      this->init();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     */
    explicit PkgDbReadOnly( const Fingerprint & fingerprint )
      : PkgDbReadOnly( fingerprint, genPkgDbName( fingerprint ).string() )
    {}


/* -------------------------------------------------------------------------- */

  /* Queries */

  // public:

    /** @return The Package Database schema version. */
    std::string getDbVersion();

    /**
     * Get the `AttrSet.id` for a given path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return A unique `row_id` ( unsigned 64bit int ) associated with @a path.
     */
    row_id getAttrSetId( const flox::AttrPath & path );

    /**
     * Check to see if database has and attribute set at @a path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return `true` iff the database has an `AttrSet` at @a path.
     */
    bool hasAttrSet( const flox::AttrPath & path );

    /**
     * Check to see if database has a complete list of packages under the
     * prefix @a path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return `true` iff the database has completely scraped the `AttrSet` at
     *          @a path.
     */
    bool completedAttrSet( const flox::AttrPath & path );

    /**
     * Get the attribute path for a given `AttrSet.id`.
     * @param row A unique `row_id` ( unsigned 64bit int ).
     * @return An attribute path prefix such as `packages.x86_64-linux` or
     *         `legacyPackages.aarch64-darwin.python3Packages`.
     */
    flox::AttrPath getAttrSetPath( row_id row );

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
     * @param row A unique `row_id` ( unsigned 64bit int ).
     * @return An attribute path such as `packages.x86_64-linux.hello` or
     *         `legacyPackages.aarch64-darwin.python3Packages.pip`.
     */
    flox::AttrPath getPackagePath( row_id row );

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


    /**
     * Return a list of `Packages.id`s for packages which satisfy a given
     * set of requirements.
     * These results may be ordered flexibly based on various query parameters.
     * TODO: document parameters effected by ordering.
     */
     std::vector<row_id> getPackages( const PkgQueryArgs & params );


    /**
     * @brief Get metadata about a single package.
     * Return `pname`, `version`, `description`, `broken`, `unfree`,
     * and `license` columns.
     * @param row A `Packages.id` to lookup.
     * @return A JSON object containing information about a package.
     */
     nlohmann::json getPackage( row_id row );


    /**
     * @brief Get metadata about a single package.
     * Return `pname`, `version`, `description`, `broken`, `unfree`,
     * and `license` columns.
     * @param row An attribute path to a package.
     * @return A JSON object containing information about a package.
     */
    nlohmann::json getPackage( const flox::AttrPath & path );


/* -------------------------------------------------------------------------- */

};  /* End class `PkgDbReadOnly' */


/* -------------------------------------------------------------------------- */

/** Restricts template parameters to classes that extend @a PkgDbReadOnly. */
  template <typename T>
concept pkgdb_typename = std::is_base_of<PkgDbReadOnly, T>::value;


/* -------------------------------------------------------------------------- */

/**
 * Predicate to detect failing SQLite3 return codes.
 * @param rcode A SQLite3 _return code_.
 * @return `true` iff @a rc is a SQLite3 error.
 */
bool isSQLError( int rcode );


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
