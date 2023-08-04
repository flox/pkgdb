/* ========================================================================== *
 *
 * @file main.cc
 *
 * @brief Executable exposing CRUD operations for package metadata.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <stdlib.h>
#include <list>
#include <filesystem>
#include <assert.h>
#include <queue>
#include <functional>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "flox/flox-flake.hh"
#include "pkg-db.hh"


/* -------------------------------------------------------------------------- */

using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;
using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using AttrPath    = std::vector<std::string>;
using Target      = std::pair<AttrPath,Cursor>;
using Todos       = std::queue<Target,std::list<Target>>;


/* -------------------------------------------------------------------------- */

/**
 * Scrape package definitions from an attribute set, adding any attributes
 * marked with `recurseForDerivatsions = true` to @a todo list.
 * @param db     Database to write to.
 * @param syms   Symbol table from @a cursor evaluator.
 * @param prefix Attribute path to scrape.
 * @param cursor `nix` evaluator cursor associated with @a prefix
 * @param todo Queue to add `recurseForDerivations = true` cursors to so they
 *             may be scraped by later invocations.
 */
  void
scrape(       flox::pkgdb::PkgDb & db
      ,       nix::SymbolTable   & syms
      , const AttrPath           & prefix
      ,       Cursor               cursor
      ,       Todos              & todo
      )
{

  bool tryRecur = prefix[0] != "packages";

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "evaluating package set '%s'"
            , nix::concatStringsSep( ".", prefix )
            )
  );

  /* Lookup/create the `pathId' for for this attr-path in our DB.
   * This must be done before starting a transaction in the database
   * because it may need to read/write multiple times. */
  flox::pkgdb::row_id parentId = db.addOrGetPackageSetId( prefix );

  /* Start a transaction */
  sqlite3pp::transaction txn( db.db );

  /* Scrape loop over attrs */
  for ( nix::Symbol & aname : cursor->getAttrs() )
    {
      const std::string pathS =
        nix::concatStringsSep( ".", prefix ) + "." + syms[aname];

      if ( syms[aname] == "recurseForDerivations" ) { continue; }

      nix::Activity act(
        * nix::logger
      , nix::lvlTalkative
      , nix::actUnknown
      , nix::fmt( "\tevaluating attribute '%s'", pathS )
      );

      try
        {
          Cursor child = cursor->getAttr( aname );
          if ( child->isDerivation() )
            {
              db.addPackage( parentId, syms[aname], child );
              continue;
            }
          if ( ! tryRecur ) { continue; }
          if ( auto m = child->maybeGetAttr( "recurseForDerivations" );
                    ( m != nullptr ) && m->getBool()
                  )
            {
              AttrPath path = prefix;
              path.emplace_back( syms[aname] );
              nix::logger->log( nix::lvlTalkative
                              , nix::fmt( "\tpushing target '%s'", pathS )
                              );
              todo.push( std::make_pair( std::move( path ), child ) );
            }
        }
      catch( const nix::EvalError & e )
        {
          /* Ignore errors in `legacyPackages' and `catalog' */
          if ( tryRecur )
            {
              /* Only print eval errors in "debug" mode. */
              nix::ignoreException( nix::lvlDebug );
            }
          else
            {
              txn.rollback();  /* Revert transaction changes */
              throw e;
            }
        }
    }

  txn.commit();  /* Commit transaction changes */

}



/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{

  /* Define arg parsers. */

  argparse::ArgumentParser prog( "pkgdb", FLOX_PKGDB_VERSION );
  prog.add_description( "CRUD operations for package metadata" );

  /* Nix verbosity levels for reference:
   *   typedef enum {
   *     lvlError = 0   ( --quiet --quiet --quiet )
   *   , lvlWarn        ( --quiet --quiet )
   *   , lvlNotice      ( --quiet )
   *   , lvlInfo        ( **Default** )
   *   , lvlTalkative   ( -v  )
   *   , lvlChatty      ( -vv   | --debug --quiet )
   *   , lvlDebug       ( -vvv  | --debug )
   *   , lvlVomit       ( -vvvv | --debug -v )
   *   } Verbosity;
   */
  nix::Verbosity verbosity = nix::lvlInfo;  /* Nix's default as well. */

  /* Add `-v+,--verbose' and `-q+,--quiet' to a parser. */
  auto addVerbosity = [&]( argparse::ArgumentParser & parser )
  {
    parser.add_argument( "-q", "--quiet" )
      .help(
        "Decreate the logging verbosity level. May be used up to 3 times."
      ).action(
        [&]( const auto & )
        {
          verbosity =
            ( verbosity <= nix::lvlError ) ? nix::lvlError
                                           : (nix::Verbosity) ( verbosity - 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append()
    ;
    parser.add_argument( "-v", "--verbose" )
      .help( "Increase the logging verbosity level. May be up to 4 times." )
      .action(
        [&]( const auto & )
        {
          verbosity =
            ( nix::lvlVomit <= verbosity ) ? nix::lvlVomit
                                           : (nix::Verbosity) ( verbosity + 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append()
    ;
  };

  addVerbosity( prog );

  argparse::ArgumentParser cmdScrape( "scrape" );
  cmdScrape.add_description( "Scrape a flake and emit a SQLite3 DB" );

  addVerbosity( cmdScrape );

  cmdScrape.add_argument( "-d", "--database" )
    .help( "Use database at PATH" )
    .default_value( "" )
    .metavar( "PATH" )
    .nargs( 1 )
  ;

  cmdScrape.add_argument( "-f", "--force" )
    .help( "Force re-evaluation of prefix" )
    .default_value( false )
    .implicit_value( true )
    .nargs( 0 )
  ;

  cmdScrape.add_argument( "flake-ref" )
    .help( "A flake-reference URI string ( preferably locked )" )
    .required()
    .metavar( "FLAKE-REF" )
  ;

  cmdScrape.add_argument( "attr-path" )
    .help( "Attribute path to scrape" )
    .metavar( "ATTR" )
    .remaining()
    .default_value( std::list<std::string> {
      "packages"
    , nix::settings.thisSystem.get()
    } )
  ;

  prog.add_subparser( cmdScrape );


/* -------------------------------------------------------------------------- */

  /* Parse Args */

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const std::runtime_error & err )
    {
      std::cerr << err.what() << std::endl;
      std::cerr << prog;
      return EXIT_FAILURE;
    }

  // TODO: remove when you have more commands.
  assert( prog.is_subcommand_used( "scrape" ) );


/* -------------------------------------------------------------------------- */

  /* Initialize `nix' */

  /* Assign verbosity to `nix' global setting */
  nix::verbosity = verbosity;
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
  /* Suppress benign warnings about `nix.conf'. */
  nix::verbosity = nix::lvlError;
  nix::initPlugins();
  /* Restore verbosity to `nix' global setting */
  nix::verbosity = verbosity;

  // TODO: make this an option. It risks making cross-system eval impossible.
  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.setDefault( true );

  /* TODO: --store PATH */
  nix::ref<nix::Store> store = nix::ref<nix::Store>( nix::openStore() );

  std::shared_ptr<nix::EvalState> state =
    std::make_shared<nix::EvalState>( std::list<std::string> {}, store, store );
  state->repair = nix::NoRepair;


/* -------------------------------------------------------------------------- */

  /* Fetch and lock our flake. */

  std::string   refStr = cmdScrape.get<std::string>( "flake-ref" );
  nix::FlakeRef ref    =
    ( refStr.find( '{' ) != refStr.npos )
    ? nix::FlakeRef::fromAttrs(
        nix::fetchers::jsonToAttrs( nlohmann::json::parse( refStr ) )
      )
    : nix::parseFlakeRef( refStr );

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "fetching flake '%s'", ref.to_string() )
  );
  flox::FloxFlake flake( (nix::ref<nix::EvalState>) state, ref );
  nix::logger->stopActivity( act.id );

  if ( ! flake.lockedFlake.flake.lockedRef.input.hasAllInfo() )
    {
      if ( nix::lvlWarn <= nix::verbosity )
        {
          nix::logger->warn(
            "flake-reference is unlocked/dirty - "
            "resulting DB may not be cached."
          );
        }
    }


/* -------------------------------------------------------------------------- */

  /* Open our database. */

  /* Maybe use path from arguments. */
  std::string dbPathStr;
  if ( cmdScrape.is_used( "-d" ) )
    {
      dbPathStr = cmdScrape.get<std::string>( "-d" );
      if ( ! dbPathStr.empty() ) { dbPathStr = nix::absPath( dbPathStr ); }
    }

  /* Fallback to generated path under `XDG_CACHE_DIR/flox/pkgdb-v0/' */
  if ( dbPathStr.empty() )
    {
      dbPathStr = flox::pkgdb::genPkgDbName( flake.lockedFlake );
    }

  /* Create directories if needed. */
  std::filesystem::path dbPath( dbPathStr );
  if ( ! std::filesystem::exists( dbPath.parent_path() ) )
    {
      std::filesystem::create_directories( dbPath.parent_path() );
    }

  /* Open/Create database. */
  flox::pkgdb::PkgDb db( flake.lockedFlake, dbPathStr );


/* -------------------------------------------------------------------------- */

  /* Set attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable". */

  AttrPath attrPath = cmdScrape.get<std::vector<std::string>>( "attr-path" );
  if ( attrPath.size() < 2 )
    {
      attrPath.push_back( nix::settings.thisSystem.get() );
    }
  if ( ( attrPath.size() < 3 ) && ( attrPath[0] == "catalog" ) )
    {
      attrPath.push_back( "stable" );
    }


/* -------------------------------------------------------------------------- */

  /* If we haven't processed this prefix before or `--force' was given, open the
   * eval cache and start scraping. */

  if ( cmdScrape.get<bool>( "-f" ) || ( ! db.hasPackageSet( attrPath ) ) )
    {
      std::vector<nix::Symbol> symbolPath;
      for ( const auto & a : attrPath )
        {
          symbolPath.emplace_back( flake.state->symbols.create( a ) );
        }

      Todos todo;
      if ( MaybeCursor root = flake.maybeOpenCursor( symbolPath );
           root != nullptr
         )
        {
          todo.push( std::make_pair( std::move( attrPath ), (Cursor) root ) );
        }

      while ( ! todo.empty() )
        {
          auto & [prefix, cursor] = todo.front();
          scrape( db, flake.state->symbols, prefix, cursor, todo );
          todo.pop();
        }
    }


/* -------------------------------------------------------------------------- */

  /* Print path to database. */
  std::cout << dbPathStr << std::endl;

  return EXIT_SUCCESS;  /* GG, GG */

}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
