/* ========================================================================== *
 *
 * @file flox/pkgdb/write.hh
 *
 * @brief Interfaces for writing to a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/pkgdb/read.hh"

/* -------------------------------------------------------------------------- */

/* This is passed in by `make' and is set by `<pkgdb>/version' */
#ifndef FLOX_PKGDB_VERSION
#  define FLOX_PKGDB_VERSION  "NO.VERSION"
#endif
#define FLOX_PKGDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb {

/* -------------------------------------------------------------------------- */

using Target = std::pair<flox::AttrPath, flox::Cursor>;
using Todos  = std::queue<Target, std::list<Target>>;


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDb : public PkgDbReadOnly {

/* -------------------------------------------------------------------------- */

  /* Internal Helpers */

  protected:

    /** Create tables in database if they do not exist. */
    void initTables();

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
     * Does NOT attempt to create a database if one does not exist.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( std::string_view dbPath ) : PkgDbReadOnly()
    {
      this->dbPath = dbPath;
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          throw PkgDbReadOnly::NoSuchDatabase( * ( (PkgDbReadOnly *) this ) );
        }
      this->db.connect( this->dbPath.c_str()
                      , SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE
                      );
      this->initTables();
      this->loadLockedFlake();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( const Fingerprint      & fingerprint
         ,       std::string_view   dbPath
         )
      : PkgDbReadOnly()
    {
      this->dbPath      = dbPath;
      this->fingerprint = fingerprint;
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          throw PkgDbReadOnly::NoSuchDatabase( * ( (PkgDbReadOnly *) this ) );
        }
      this->db.connect( this->dbPath.c_str()
                      , SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE
                      );
      this->initTables();
      this->loadLockedFlake();
    }

    /**
     * Opens a DB directly by its fingerprint hash.
     * Does NOT attempt to create a database if one does not exist.
     * @param fingerprint Unique hash associated with locked flake.
     */
    PkgDb( const Fingerprint & fingerprint )
      : PkgDb( fingerprint, genPkgDbName( fingerprint ) )
    {}

    /**
     * Opens a DB associated with a locked flake.
     * Creates database if one does not exist.
     * @param flake Flake associated with the db. Used to write input metadata.
     * @param dbPath Absolute path to database file.
     */
    PkgDb( const nix::flake::LockedFlake & flake
         ,       std::string_view          dbPath
         )
      : PkgDbReadOnly()
    {
      this->dbPath      = dbPath;
      this->fingerprint = flake.getFingerprint();
      this->db.connect( this->dbPath.c_str()
                      , SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE
                      );
      initTables();
      this->lockedRef = {
        flake.flake.lockedRef.to_string()
      , nix::fetchers::attrsToJSON( flake.flake.lockedRef.toAttrs() )
      };
      writeInput();
    }

    /**
     * Opens a DB associated with a locked flake.
     * Creates database if one does not exist.
     * @param flake Flake associated with the db. Used to write input metadata.
     */
    PkgDb( const nix::flake::LockedFlake & flake )
      : PkgDb( flake, genPkgDbName( flake.getFingerprint() ) )
    {}


/* -------------------------------------------------------------------------- */

  /* Basic Operations */

  public:

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

/* Updates */

/**
 * Update the `done` column for an attribute set and all of its
 * children recursively.
 * @param prefix Attribute set prefix to be updated.
 * @param done Value to update `done` column to.
 */
void setPrefixDone( const flox::AttrPath & prefix, bool done );


/* -------------------------------------------------------------------------- */

/**
 * Scrape package definitions from an attribute set, adding any attributes
 * marked with `recurseForDerivatsions = true` to @a todo list.
 * @param syms Symbol table from @a cursor evaluator.
 * @param prefix Attribute path to scrape.
 * @param cursor `nix` evaluator cursor associated with @a prefix
 * @param todo Queue to add `recurseForDerivations = true` cursors to so they
 *             may be scraped by later invocations.
 */
  void
scrape(       nix::SymbolTable & syms
      , const flox::AttrPath   & prefix
      ,       flox::Cursor       cursor
      ,       Todos            & todo
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
