/* ========================================================================== *
 *
 * @file flox/pkgdb/input.hh
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

/** @brief A @a RegistryInput that opens a @a PkgDb associated with a flake. */
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


  public:

    /** The name of the input, used to emit output with shortnames. */
    std::optional<std::string> name;


  private:

    /**
     * @brief Prepare database handles for use.
     *
     * Upon exiting a compatible read-only database connection will be open
     * with the `LockedFlake` and `DbVersions` tables created.
     *
     * If the database does not exist it will be created.
     *
     * If the database `VIEW`s schemas are out of date they will be updated.
     *
     * If the database `TABLE`s schemas are out of date the database will be
     * deleted and recreated.
     */
    void init();


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
     * @param name Name of the input ( empty implies N/A ).
     */
    PkgDbInput(       nix::ref<nix::Store>  & store
              , const RegistryInput         & input
              ,       std::filesystem::path   dbPath
              , const db_path_tag           & /* unused */
              , const std::string           & name = ""
              )
      : FloxFlakeInput( store, input )
      , dbPath( std::move( dbPath ) )
      , name( name.empty() ? std::nullopt : std::make_optional( name ) )
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
     * @param name Name of the input ( empty implies N/A ).
     */
    PkgDbInput(       nix::ref<nix::Store>  & store
              , const RegistryInput         & input
              , const std::filesystem::path & cacheDir = getPkgDbCachedir()
              , const std::string           & name = ""
              )
      : FloxFlakeInput( store, input )
      , dbPath(
          genPkgDbName( this->getFlake()->lockedFlake.getFingerprint()
                      , cacheDir
                      )
        )
      , name( name.empty() ? std::nullopt : std::make_optional( name ) )
    {
      this->init();
    }


    /**
     * @return The read-only database connection handle.
     */
      [[nodiscard]]
      nix::ref<PkgDbReadOnly>
    getDbReadOnly() const
    {
      return static_cast<nix::ref<PkgDbReadOnly>>( this->dbRO );
    }


    /**
     * @brief Open a read/write database connection if one is not open, and
     *        return a handle.
     */
    [[nodiscard]] nix::ref<PkgDb> getDbReadWrite();


    /** @brief Close the read/write database connection if it is open. */
    void closeDbReadWrite() { this->dbRW = nullptr; }


    /** @return Filesystem path to the flake's package database. */
    [[nodiscard]]
    std::filesystem::path getDbPath() const { return this->dbPath; }


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
    void scrapePrefix( const flox::AttrPath & prefix );


    /**
     * @brief Scrape all prefixes indicated by @a InputPreferences for
     *        @a systems.
     * @param systems Systems to be scraped.
     */
    void scrapeSystems( const std::vector<std::string> & systems );


    /**
     * @brief Get an identifier for this input.
     * @return The shortname of this input, or its locked flake-ref.
     */
      std::string
    getNameOrURL()
    {
      return this->name.value_or(
        this->getFlake()->lockedFlake.flake.lockedRef.to_string()
      );
    }


    /**
     * @brief Get a JSON representation of a row in the database.
     */
    nlohmann::json getRowJSON( row_id row );


/* -------------------------------------------------------------------------- */

};  /* End struct `PkgDbInput' */


/* -------------------------------------------------------------------------- */

/** @brief Factory for @a PkgDbInput. */
class PkgDbInputFactory {

  private:
    nix::ref<nix::Store>  store;    /**< `nix` store connection. */
    std::filesystem::path cacheDir; /**< Cache directory. */

  public:
    using input_type = PkgDbInput;

    /** @brief Construct a factory using a `nix` evaluator. */
    explicit PkgDbInputFactory(
      nix::ref<nix::Store>  & store
    , std::filesystem::path   cacheDir = getPkgDbCachedir()
    ) : store( store )
      , cacheDir( std::move( cacheDir ) )
    {}

    /** @brief Construct an input from a @a RegistryInput. */
      [[nodiscard]]
      std::shared_ptr<PkgDbInput>
    mkInput( const std::string & name, const RegistryInput & input )
    {
      return std::make_shared<PkgDbInput>(
        this->store
      , input
      , this->cacheDir
      , name
      );
    }

};  /* End class `PkgDbInputFactory' */


static_assert( registry_input_factory<PkgDbInputFactory> );


/* -------------------------------------------------------------------------- */

/**
 * @brief Provides a registry of @a PkgDb managers.
 *
 * Derived classes must provide their own @a getRegistryRaw and @a getSystems
 * implementations to support @a initRegistry and @a scrapeIfNeeded.
 */
struct PkgDbRegistryMixin : virtual public NixState {

  protected:

    bool force = false;  /**< Whether to force re-evaluation of flakes. */

    std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;

    /** Initialize @a registry member from @a params.registry. */
    void initRegistry();

    /**
     * Lazily perform scraping on input flakes.
     * If scraping is necessary temprorary read/write handles are opened for
     * those flakes and closed before returning from this function.
     */
    void scrapeIfNeeded();

    /** @return A raw registry used to initialize. */
    [[nodiscard]] virtual RegistryRaw getRegistryRaw() = 0;

    /** @return A list of systems to be scraped. */
    [[nodiscard]] virtual std::vector<std::string> & getSystems() = 0;

};  /* End struct `PkgDbRegistryMixin' */


/* -------------------------------------------------------------------------- */


}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
