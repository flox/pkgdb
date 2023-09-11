/* ========================================================================== *
 *
 * @file pkgdb/input.cc
 *
 * @brief Helpers for managing package database inputs and state.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/pkgdb/input.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

  void
PkgDbInput::init()
{
  /* Initialize DB if missing. */
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
  SqlVersions dbVersions = this->dbRO->getDbVersion();
  if ( dbVersions.tables != sqlVersions.tables )
    {
      nix::logger->log(
        nix::lvlTalkative
      , nix::fmt( "Clearing outdated database '%s'", this->dbPath.string() )
      );
      std::filesystem::remove( this->dbPath );
      PkgDb( this->getFlake()->lockedFlake, this->dbPath.string() );
    }
  else if ( dbVersions.views != sqlVersions.views )
    {
      nix::logger->log(
        nix::lvlTalkative
      , nix::fmt( "Updating outdated database views '%s'"
                , this->dbPath.string()
                )
      );
      PkgDb( this->getFlake()->lockedFlake, this->dbPath.string() );
    }

  /* If the schema version is still wrong throw an error, but we don't
   * expect this to actually occur. */
  dbVersions = this->dbRO->getDbVersion();
  if ( dbVersions != sqlVersions )
    {
      throw PkgDbException(
        this->dbPath
      , nix::fmt( "Incompatible Flox PkgDb schema versions ( %u, %u )"
                , dbVersions.tables
                , dbVersions.views
                )
      );
    }
}


/* -------------------------------------------------------------------------- */

  nix::ref<PkgDb>
PkgDbInput::getDbReadWrite()
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


/* -------------------------------------------------------------------------- */

  void
PkgDbInput::scrapePrefix( const flox::AttrPath & prefix )
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

  void
PkgDbInput::scrapeSystems( const std::vector<std::string> & systems )
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

  nlohmann::json
PkgDbInput::getRowJSON( row_id row )
{
  nix::ref<PkgDbReadOnly> db = this->getDbReadOnly();
  nlohmann::json rsl = db->getPackage( row );
  rsl.emplace( "input", this->getNameOrURL() );
  rsl.emplace( "path",  db->getPackagePath( row ) );
  return rsl;
}


/* -------------------------------------------------------------------------- */

  void
PkgDbRegistryMixin::initRegistry()
{
  nix::ref<nix::Store> store = this->getStore();
  pkgdb::PkgDbInputFactory factory( store );  // TODO: cacheDir
  this->registry = std::make_shared<Registry<PkgDbInputFactory>>(
    this->getRegistryRaw()
  , factory
  );
}


/* -------------------------------------------------------------------------- */

  void
PkgDbRegistryMixin::scrapeIfNeeded()
{
  assert( this->registry != nullptr );
  for ( auto & [name, input] : * this->registry )
    {
      input->scrapeSystems( this->getSystems() );
    }
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
