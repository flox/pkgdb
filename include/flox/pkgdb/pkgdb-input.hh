/* ========================================================================== *
 *
 * @file flox/pkgdb/pkgdb-input.hh
 *
 * @brief A @a RegistryInput that opens a @a PkgDb associated with a flake.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/pkgdb/write.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/** A @a RegistryInput that opens a @a PkgDb associated with a flake. */
class PkgDbInput : public FloxFlakeInput {

  private:

    /** Path to the flake's pkgdb SQLite3 file. */
    std::filesystem::path dbPath;

    /**
     * @brief A read-only database connection that remains open for the lifetime
     *        of @a this object.
     */
    std::shared_ptr<PkgDbReadOnly> dbRO;

    /**
     * @brief A read/write database connection that may be opened and closed as
     *        needed using @a getDbReadWrite and @a closeDbReadWrite.
     */
    std::shared_ptr<PkgDb> dbRW;

  public:

    PkgDbInput(       nix::ref<nix::EvalState> & state
              , const RegistryInput            & input
              )
      : FloxFlakeInput( state, input )
      , dbPath( genPkgDbName( this->flake->lockedFlake.getFingerprint() ) )
    {

      /* Initialized DB if missing. */
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          std::filesystem::create_directories( this->dbPath.parent_path() );
          nix::logger->log(
            nix::lvlTalkative
          , nix::fmt( "Creating database '%s'", this->dbPath.string() )
          );
          PkgDb( this->flake->lockedFlake, this->dbPath.string() );
        }

      this->dbRO = std::make_shared<PkgDbReadOnly>(
        this->flake->lockedFlake.getFingerprint()
      , this->dbPath.string()
      );

      /* If the schema version is bad, delete the DB so it will be recreated. */
      if ( this->dbRO->getDbVersion() != FLOX_PKGDB_SCHEMA_VERSION )
        {
          nix::logger->log(
            nix::lvlTalkative
          , nix::fmt( "Clearing outdated database '%s'", this->dbPath.string() )
          );
          std::filesystem::remove( this->dbPath );
          PkgDb( this->flake->lockedFlake, this->dbPath.string() );
        }

      /* If the schema version is still wrong throw an error, but we don't
       * expect this to actually occur. */
      if ( this->dbRO->getDbVersion() != FLOX_PKGDB_SCHEMA_VERSION )
        {
          throw PkgDbException(
            this->dbPath
          , "Incompatible Flox PkgDb schema version" +
            this->dbRO->getDbVersion()
          );
        }

    }


    /**
     * @return The read-only database connection handle.
     */
      [[nodiscard]]
      nix::ref<PkgDbReadOnly>
    getDbReadOnly() const
    {
      return (nix::ref<PkgDbReadOnly>) this->dbRO;
    }

    /**
     * @brief Open a read/write database connection if one is not open, and
     * return a handle.
     */
      [[nodiscard]]
      nix::ref<PkgDb>
    getDbReadWrite()
    {
      if ( this->dbRW == nullptr )
        {
          this->dbRW = std::make_shared<PkgDb>(
            this->flake->lockedFlake
          , this->dbPath.string()
          );
        }
      return (nix::ref<PkgDb>) this->dbRW;
    }


    /** Close the read/write database connection if it is open. */
    void closeDbReadWrite() { this->dbRW = nullptr; }

    /** @return Filesystem path to the flake's package database. */
    [[nodiscard]]
    std::filesystem::path getDbPath() const { return this->dbPath; }

};  /* End struct `PkgDbInput' */


/* -------------------------------------------------------------------------- */


}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
