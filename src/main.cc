/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <stdlib.h>
#include <list>
#include <filesystem>

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
  argparse::ArgumentParser prog( "pkgdb", FLOX_PKGDB_VERSION );
  prog.add_description( "CRUD operations for package metadata" );

  argparse::ArgumentParser cmdScrape( "scrape" );
  cmdScrape.add_description( "Scrape a flake and emit a SQLite3 DB" );

  cmdScrape.add_argument( "--system" )
    .help( "indicate that a system should be scraped" )
    .append()
    .default_value( std::list<std::string> { nix::settings.thisSystem.get() } )
  ;

  cmdScrape.add_argument( "-o", "--out" )
    .help( "write database to FILE" )
    .default_value( "" )
    .metavar( "FILE" )
    .nargs( 1 )
  ;

  cmdScrape.add_argument( "flake-ref" )
    .help( "A flake-reference URI string ( preferably locked )" )
    .required()
    .metavar( "FLAKE-REF" )
  ;

  prog.add_subparser( cmdScrape );


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

  auto systems = cmdScrape.get<std::list<std::string>>( "--system" );

  std::string   refStr = cmdScrape.get<std::string>( "flake-ref" );
  nix::FlakeRef ref    =
    ( refStr.find( '{' ) != refStr.npos )
    ? nix::FlakeRef::fromAttrs(
        nix::fetchers::jsonToAttrs( nlohmann::json::parse( refStr ) )
      )
    : nix::parseFlakeRef( refStr );

  char * outFile = nullptr;
  try
    {
      outFile = cmdScrape.get<char *>( "-o" );
      if ( std::string_view( outFile ).empty() ) { outFile = nullptr; }
    }
  catch( const std::runtime_error & e )
    {
      /* Ignored: Uses fingerprint */
    }


/* -------------------------------------------------------------------------- */

  /* Boilerplate */

  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
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
      nix::logger->warn(
        "flake-reference is unlocked/dirty - resulting DB may not be cached."
      );
    }


  std::string dbPathStr( flox::pkgdb::getPkgDbName( flake.lockedFlake ) );

  std::filesystem::path dbPath( dbPathStr );
  if ( ! std::filesystem::exists( dbPath.parent_path() ) )
    {
      std::filesystem::create_directories( dbPath.parent_path() );
    }


  flox::pkgdb::PkgDb db( flake.lockedFlake );

  /* TODO: handle prefixes/system */

  using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;
  using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;

  for ( auto & system : systems  )
    {
      std::vector<std::string>      attrPathS = { "legacyPackages", system };
      std::vector<std::string_view> attrPath  = { "legacyPackages", system };

      flox::pkgdb::row_id parentId = db.addOrGetPackageSetId( attrPath );

      nix::Activity act(
        * nix::logger
      , nix::lvlInfo
      , nix::actUnknown
      , nix::fmt( "evaluating '%s'", nix::concatStringsSep( ".", attrPathS ) )
      );

      MaybeCursor root = flake.maybeOpenCursor( {
          flake.state->symbols.create( "legacyPackages" )
        , flake.state->symbols.create( system )
        }
      );
      if ( root == nullptr ) { continue; }

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
            , nix::concatStringsSep( ".", attrPathS )
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

  std::cout << dbPathStr << std::endl;


/* -------------------------------------------------------------------------- */

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
