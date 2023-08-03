/* ========================================================================== *
 *
 * @file main.cc
 *
 * @brief executable exposing CRUD operations for package metadata.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <stdlib.h>
#include <list>
#include <filesystem>
#include <assert.h>

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

  /* Set attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable". */
  std::vector<std::string> attrPath =
    cmdScrape.get<std::vector<std::string>>( "attr-path" );
  if ( attrPath.size() < 2 )
    {
      attrPath.push_back( nix::settings.thisSystem.get() );
    }
  if ( ( attrPath.size() < 3 ) && ( attrPath[0] == "catalog" ) )
    {
      attrPath.push_back( "stable" );
    }


  std::string   refStr = cmdScrape.get<std::string>( "flake-ref" );
  nix::FlakeRef ref    =
    ( refStr.find( '{' ) != refStr.npos )
    ? nix::FlakeRef::fromAttrs(
        nix::fetchers::jsonToAttrs( nlohmann::json::parse( refStr ) )
      )
    : nix::parseFlakeRef( refStr );

  /* Assign verbosity to `nix' global setting */
  nix::verbosity = verbosity;


/* -------------------------------------------------------------------------- */

  /* Boilerplate */


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

  flox::FloxFlake flake( (nix::ref<nix::EvalState>) state, ref );

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

  std::string dbPathStr;
  if ( cmdScrape.is_used( "-d" ) )
    {
      dbPathStr = cmdScrape.get<std::string>( "-d" );
      if ( ! dbPathStr.empty() ) { dbPathStr = nix::absPath( dbPathStr ); }
    }

  if ( dbPathStr.empty() )
    {
      dbPathStr = flox::pkgdb::genPkgDbName( flake.lockedFlake );
    }

  std::filesystem::path dbPath( dbPathStr );
  if ( ! std::filesystem::exists( dbPath.parent_path() ) )
    {
      std::filesystem::create_directories( dbPath.parent_path() );
    }

  flox::pkgdb::PkgDb db( flake.lockedFlake, dbPathStr );

  /* TODO: handle prefixes/system */

  using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;
  using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;

  std::vector<nix::Symbol>      attrPathS;
  std::vector<std::string_view> attrPathV;
  for ( const auto & a : attrPath )
    {
      attrPathS.push_back( flake.state->symbols.create( a ) );
      attrPathV.emplace_back( a );
    }

  flox::pkgdb::row_id parentId = db.addOrGetPackageSetId( attrPathV );

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "evaluating '%s'", nix::concatStringsSep( ".", attrPath ) )
  );

  MaybeCursor root = flake.maybeOpenCursor( attrPathS );
  if ( root != nullptr )
    {
      /* Start a transaction */
      sqlite3pp::transaction txn( db.db );

      /* Scrape loop over attrs */
      for ( nix::Symbol & aname : root->getAttrs() )
        {
          nix::Activity act(
            * nix::logger
          , nix::lvlTalkative
          , nix::actUnknown
          , nix::fmt(
              "\tevaluating '%s.%s'"
            , nix::concatStringsSep( ".", attrPath )
            , flake.state->symbols[aname]
            )
          );

          try
            {
              Cursor child = root->getAttr( aname );
              /* TODO: recurseForDerivations */
              if ( ! child->isDerivation() ) { continue; }
              db.addPackage( parentId, flake.state->symbols[aname], child );
            }
          catch( nix::EvalError & err )
            {
              /* Only print eval errors in "debug" mode. */
              nix::ignoreException( nix::Verbosity::lvlDebug );
            }
        }

      txn.commit();  /* Close transaction */
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
