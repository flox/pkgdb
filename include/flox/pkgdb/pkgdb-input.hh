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
      /* Initialized DB if missing. */
      if ( ! std::filesystem::exists( this->dbPath ) )
        {
          std::filesystem::create_directories( this->dbPath.parent_path() );
          nix::logger->log(
            nix::lvlTalkative
          , nix::fmt( "Creating database '%s'", this->dbPath.string() )
          );
          PkgDb( this->getFlake()->lockedFlake, this->dbPath.string() );
        }

      this->dbRO = std::make_shared<PkgDbReadOnly>(
        this->getFlake()->lockedFlake.getFingerprint()
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
          PkgDb( this->getFlake()->lockedFlake, this->dbPath.string() );
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

    /**
     * @brief Tag used to disambiguate construction with database path and
     *        cache directory path.
     */
    struct db_path_tag  {};

    /**
     * @brief Construct a @a PkgDbInput from a @a RegistryInput and a path to
     *        the database.
     * @param store A `nix` store connection.
     * @param input A @a RegistryInput.
     * @param dbPath Path to the database.
     * @param db_path_tag Tag used to disambiguate this constructor from
     *                    other constructor which takes a cache directory.
     */
    PkgDbInput(       nix::ref<nix::Store>  & store
              , const RegistryInput         & input
              ,       std::filesystem::path   dbPath
              , const db_path_tag           & /* unused */
              )
      : FloxFlakeInput( store, input )
      , dbPath( std::move( dbPath ) )
    {
      this->init();
    }


    /**
     * @brief Construct a @a PkgDbInput from a @a RegistryInput and a path to
     *        the directory where the database should be cached.
     * @param store A `nix` store connection.
     * @param input A @a RegistryInput.
     * @param cacheDir Path to the directory where the database should
     *                 be cached.
     */
    PkgDbInput(       nix::ref<nix::Store>  & store
              , const RegistryInput         & input
              , const std::filesystem::path & cacheDir = getPkgDbCachedir()
              )
      : FloxFlakeInput( store, input )
      , dbPath(
          genPkgDbName( this->getFlake()->lockedFlake.getFingerprint()
                      , cacheDir
                      )
        )
    {
      this->init();
    }


/* -------------------------------------------------------------------------- */

    /**
     * @return The read-only database connection handle.
     */
      [[nodiscard]]
      nix::ref<PkgDbReadOnly>
    getDbReadOnly() const
    {
      return static_cast<nix::ref<PkgDbReadOnly>>( this->dbRO );
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
            this->getFlake()->lockedFlake
          , this->dbPath.string()
          );
        }
      return static_cast<nix::ref<PkgDb>>( this->dbRW );
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

      bool        wasRW    = this->dbRW != nullptr;
      MaybeCursor root     = this->getFlake()->maybeOpenCursor( prefix );

      if (root == nullptr ) { return; }

      auto   db  = this->getDbReadWrite();
      row_id row = db->addOrGetAttrSetId( prefix );

      todo.emplace(
        std::make_tuple( prefix, static_cast<flox::Cursor>( root ), row )
      );

      /* Start a transaction */
      sqlite3pp::transaction txn( db->db );
      try
        {
          while ( ! todo.empty() )
            {
              this->dbRW->scrape(
                this->getFlake()->state->symbols
              , todo.front()
              , todo
              );
              todo.pop();
            }

          /* Mark the prefix and its descendants as "done" */
          this->dbRW->setPrefixDone( row, true );
        }
      catch( const nix::EvalError & )
        {
          txn.rollback();
          /* Close the r/w connection if we opened it. */
          if ( ! wasRW ) { this->dbRW = nullptr; }
          throw;
        }

      /* Close the transaction. */
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

/** Factory for @a PkgDbInput. */
class PkgDbInputFactory {

  private:
    nix::ref<nix::Store>  store;    /**< `nix` store connection. */
    std::filesystem::path cacheDir; /**< Cache directory. */

  public:
    using input_type = PkgDbInput;

    /** Construct a factory using a `nix` evaluator. */
    explicit PkgDbInputFactory(
      nix::ref<nix::Store>  & store
    , std::filesystem::path   cacheDir = getPkgDbCachedir()
    ) : store( store )
      , cacheDir( std::move( cacheDir ) )
    {}

    /** Construct an input from a @a RegistryInput. */
      [[nodiscard]]
      std::shared_ptr<PkgDbInput>
    mkInput( const std::string & /* unused */, const RegistryInput & input )
    {
      return std::make_shared<PkgDbInput>( this->store, input, this->cacheDir );
    }

};  /* End class `PkgDbInputFactory' */


static_assert( registry_input_factory<PkgDbInputFactory> );


/* -------------------------------------------------------------------------- */


}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
