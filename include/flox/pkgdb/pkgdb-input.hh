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

/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

      void
    init()
    {
      if ( this->dbRW != nullptr ) { return; }

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



/* -------------------------------------------------------------------------- */

  public:

    PkgDbInput(       nix::ref<nix::EvalState> & state
              , const RegistryInput            & input
              )
      : FloxFlakeInput( state, input )
      , dbPath( genPkgDbName( this->flake->lockedFlake.getFingerprint() ) )
    {
    }


/* -------------------------------------------------------------------------- */

      void
    setDbPath( const std::filesystem::path & path )
    {
      this->dbRO   = nullptr;
      this->dbRW   = nullptr;
      this->dbPath = path;
    }


/* -------------------------------------------------------------------------- */

    /**
     * @return The read-only database connection handle.
     */
      [[nodiscard]]
      nix::ref<PkgDbReadOnly>
    getDbReadOnly()
    {
      if ( this->dbRO == nullptr ) { this->init(); }
      return (nix::ref<PkgDbReadOnly>) this->dbRO;
    }


/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

    /** @return Filesystem path to the flake's package database. */
    [[nodiscard]]
    std::filesystem::path getDbPath() const { return this->dbPath; }


/* -------------------------------------------------------------------------- */

    /**
     * @brief Ensure that an attribute path prefix has been scraped.
     *
     * If the prefix has been scraped no writes are performed, but if the prefix
     * has not been scraped a read/write connection will be used.
     *
     * If a read/write connection is already open when @a scrapePrefix is called
     * it will remain open, but if the connection is opened by @a scrapePrefix
     * it will be closed after scraping is completed.
     * @param prefix Attribute path to scrape.
     */
      void
    scrapePrefix( const flox::AttrPath & prefix )
    {
      if ( this->getDbReadOnly()->completedAttrSet( prefix ) ) { return; }

      Todos todo;

      if ( flox::MaybeCursor root = this->flake->maybeOpenCursor( prefix );
           root != nullptr
         )
        {
          todo.push(
            std::make_pair( prefix
                          , static_cast<flox::Cursor>( root )
                          )
          );
        }

      bool wasRW = this->dbRW != nullptr;

      /* Start a transaction */
      sqlite3pp::transaction txn( this->getDbReadWrite()->db );
      try
        {
          while ( ! todo.empty() )
            {
              auto & [prefix, cursor] = todo.front();
              this->dbRW->scrape(
                this->flake->state->symbols
              , prefix
              , cursor
              , todo
              );
              todo.pop();
            }

          /* Mark the prefix and its descendants as "done" */
          this->dbRW->setPrefixDone( prefix, true );
        }
      catch( const nix::EvalError & )
        {
          txn.rollback();
          /* Close the r/w connection if we opened it. */
          if ( ! wasRW ) { this->dbRW = nullptr; }
          throw;
        }
      txn.commit();
      /* Close the r/w connection if we opened it. */
      if ( ! wasRW ) { this->dbRW = nullptr; }

    }


/* -------------------------------------------------------------------------- */

  /**
   * @brief Scrape all prefixes indicated by @a InputPreferences for @a systems.
   * @param systems Systems to be scraped.
   */
    void
  scrapeSystems( const std::vector<std::string> & systems )
  {
    /* Set fallbacks */
    std::vector<subtree_type> subtrees = this->subtrees.value_or(
      std::vector<subtree_type> { ST_PACKAGES, ST_LEGACY, ST_CATALOG }
    );
    std::vector<std::string> stabilities =
      this->stabilities.value_or( std::vector<std::string> { "stable" } );
    /* Loop and scrape over `subtrees', `systems', and `stabilities'. */
    for ( const auto & subtree : subtrees )
      {
        flox::AttrPath prefix;
        switch ( subtree )
          {
            case ST_PACKAGES: prefix = { "packages" };         break;
            case ST_LEGACY:   prefix = { "legacyPackages" };   break;
            case ST_CATALOG:  prefix = { "catalog" };          break;
            default: throw FloxException( "Invalid subtree" ); break;
          }
        for ( const auto & system : systems )
          {
            prefix.emplace_back( system );
            if ( subtree != ST_CATALOG )
              {
                this->scrapePrefix( prefix );
              }
            else
              {
                for ( const auto & stability : stabilities )
                  {
                    prefix.emplace_back( stability );
                    this->scrapePrefix( prefix );
                    prefix.pop_back();
                  }
              }
            prefix.pop_back();
          }
      }
  }


/* -------------------------------------------------------------------------- */

};  /* End struct `PkgDbInput' */


/* -------------------------------------------------------------------------- */


}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
