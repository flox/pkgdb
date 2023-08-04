/* ========================================================================== *
 *
 * @file pkgdb.cc
 *
 * @brief Tests for `flox::pkgdb::PkgDb` interfaces.
 *
 *
 * -------------------------------------------------------------------------- */

#include <limits>
#include <iostream>
#include <stdlib.h>
#include <list>
#include <assert.h>
#include <queue>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/flox-flake.hh"
#include "pkg-db.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using flox::pkgdb::row_id;


/* -------------------------------------------------------------------------- */

#define EXPECT( EXPR )                      \
  if ( ! ( EXPR ) )                         \
    {                                       \
      std::cerr << "Expectation failed: ";  \
      std::cerr << ( # EXPR );              \
      std::cerr << std::endl;               \
      return false;                         \
    }

#define EXPECT_EQ( EXPR_A, EXPR_B )             \
  {                                             \
    auto _a = ( EXPR_A );                       \
    auto _b = ( EXPR_B );                       \
    if ( _a != _b )                             \
      {                                         \
        std::cerr << "Expectation failed: ( ";  \
        std::cerr << ( # EXPR_A );              \
        std::cerr << " ) == ( ";                \
        std::cerr << ( # EXPR_B );              \
        std::cerr << " ). Got '";               \
        std::cerr << _a;                        \
        std::cerr << "' != '";                  \
        std::cerr << _b;                        \
        std::cerr << "'" << std::endl;          \
        return false;                           \
      }                                         \
  }



/* -------------------------------------------------------------------------- */

  bool
test_addOrGetAttrSetId0( flox::pkgdb::PkgDb & db )
{
  row_id id = db.addOrGetAttrSetId( "legacyPackages" );
  EXPECT_EQ( 1, id );
  id = db.addOrGetAttrSetId( "x86_64-linux", id );
  EXPECT_EQ( 2, id );
  try
    {
      id = db.addOrGetAttrSetId( "phony", 99999999 );
      std::cerr << id << std::endl;
      return false;
    }
  catch( const flox::pkgdb::PkgDbException & e ) {}
  catch( const std::exception & e )
    {
      std::cerr << e.what() << std::endl;
      return false;
    }
  return true;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

/* -------------------------------------------------------------------------- */

  nix::Verbosity verbosity;
  if ( ( 1 < argc ) && ( std::string_view( argv[1] ) == "-v" ) )
    {
      verbosity = nix::lvlTalkative;
    }
  else
    {
      verbosity = nix::lvlWarn;
    }

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

  nix::evalSettings.enableImportFromDerivation = false;
  nix::evalSettings.pureEval                   = false;
  nix::evalSettings.useEvalCache               = true;

  nix::ref<nix::Store> store = nix::ref<nix::Store>( nix::openStore() );

  std::shared_ptr<nix::EvalState> state =
    std::make_shared<nix::EvalState>( std::list<std::string> {}, store, store );
  state->repair = nix::NoRepair;


/* -------------------------------------------------------------------------- */

  auto [fd, path] = nix::createTempFile( "test-pkgdb.sql" );
  fd.close();

  nix::FlakeRef ref = nix::parseFlakeRef( nixpkgsRef );

  nix::Activity act(
    * nix::logger
  , nix::lvlInfo
  , nix::actUnknown
  , nix::fmt( "fetching flake '%s'", ref.to_string() )
  );
  flox::FloxFlake flake( (nix::ref<nix::EvalState>) state, ref );
  nix::logger->stopActivity( act.id );


/* -------------------------------------------------------------------------- */

  {

    flox::pkgdb::PkgDb db( flake.lockedFlake, path );

    RUN_TEST( addOrGetAttrSetId0, db );

  }


/* -------------------------------------------------------------------------- */

  /* XXX: You may find it useful to preserve the file and print it for some
   *      debugging efforts. */
  std::filesystem::remove( path );
  //std::cerr << path << std::endl;


/* -------------------------------------------------------------------------- */

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
