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
#include "flox/util.hh"
#include "pkgdb.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using flox::pkgdb::row_id;


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
      /* Ensure we throw an error for undefined `AttrSet.id' parents. */
      id = db.addOrGetAttrSetId( "phony", 99999999 );
      std::cerr << id << std::endl;
      return false;
    }
  catch( const flox::pkgdb::PkgDbException & e ) { /* Expected */ }
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
  flox::NixState nstate( verbosity );


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
  flox::FloxFlake flake( (nix::ref<nix::EvalState>) nstate.state, ref );
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
