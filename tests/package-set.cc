/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <optional>
#include "test.hh"
#include "flox/raw-package-set.hh"
#include "flox/types.hh"
#include "descriptor.hh"
#include "flox/db-package-set.hh"
#include "flox/flake-package-set.hh"
#include "flox/cached-package-set.hh"
#include "flox/resolver-state.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_cachePackageSet1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet fps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  DbPackageSet    dps = cachePackageSet( fps );
  return ( dps.size() == fps.size() ) && ( dps.size() == unbrokenPkgCount );
}


/* -------------------------------------------------------------------------- */

  bool
test_RawPackageSet_iterator1()
{
  RawPackageMap pkgs {
    { { "hello" }
    , nix::make_ref<RawPackage>(
        (std::vector<std::string_view>) {
          "legacyPackages", "x86_64-linux", "hello"
        }
      , "hello-2.12.1"
      , "hello"
      , "2.12.1"
      , "2.12.1"
      , "GPL-3.0-or-later"
      , (std::vector<std::string_view>) { "out" }
      , (std::vector<std::string_view>) { "out" }
      , std::make_optional( false )
      , std::make_optional( false )
      , true
      , true
      , true
      )
    }
  };
  RawPackageSet ps {
    std::move( pkgs )
  , ST_LEGACY
  , "x86_64-linux"
  , std::nullopt
  , nix::parseFlakeRef( nixpkgsRef )
  };

  for ( auto it = ps.begin(); it != ps.end(); ++it )
    {
      if ( ( * it ).getPname() != "hello" ) { return false; }
      break;
    }
  for ( auto & p : ps )
    {
      if ( p.getPname() != "hello" ) { return false; }
      break;
    }

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_RawPackageSet_addPackage1()
{
  RawPackage pkg(
    (std::vector<std::string_view>) {
      "legacyPackages", "x86_64-linux", "hello"
    }
  , "hello-2.12.1"
  , "hello"
  , "2.12.1"
  , "2.12.1"
  , "GPL-3.0-or-later"
  , (std::vector<std::string_view>) { "out" }
  , (std::vector<std::string_view>) { "out" }
  , std::make_optional( false )
  , std::make_optional( false )
  , true
  , true
  , true
  );
  RawPackageSet ps {
    {}
  , ST_LEGACY
  , "x86_64-linux"
  , std::nullopt
  , nix::parseFlakeRef( nixpkgsRef )
  };

  ps.addPackage( std::move( pkg ) );

  for ( auto it = ps.begin(); it != ps.end(); ++it )
    {
      if ( ( * it ).getPname() != "hello" ) { return false; }
      break;
    }
  for ( auto & p : ps )
    {
      if ( p.getPname() != "hello" ) { return false; }
      break;
    }

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_DbPackageSet_iterator1( std::shared_ptr<nix::flake::LockedFlake> flake )
{
  DbPackageSet ps( flake, ST_LEGACY, "x86_64-linux" );
  size_t c1 = 0;
  size_t c2 = 0;
  for ( auto it = ps.begin(); it != ps.end(); ++it, ++c1 ) {}
  for ( auto & p : ps ) { ++c2; }
  // TODO: after DB is populated by new routines change to `fullPkgCount'
  return ( c1 == c2 ) && ( c1 == unbrokenPkgCount );
}


/* -------------------------------------------------------------------------- */

  bool
test_DbPackageSet_size1( std::shared_ptr<nix::flake::LockedFlake> flake )
{
  DbPackageSet ps( flake, ST_LEGACY, "x86_64-linux" );
  size_t c = 0;
  for ( auto & p : ps ) { ++c; }
  // TODO: after DB is populated by new routines change to `fullPkgCount'
  return ( c == ps.size() ) && ( c == unbrokenPkgCount );
}


/* -------------------------------------------------------------------------- */

  bool
test_FlakePackageSet_size1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet ps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  return ps.size() == unbrokenPkgCount;
}


/* -------------------------------------------------------------------------- */

  bool
test_FlakePackageSet_hasRelPath1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet ps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  return ps.hasRelPath( { "hello" } );
}


/* -------------------------------------------------------------------------- */

  bool
test_FlakePackageSet_maybeGetRelPath1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet ps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  return ps.maybeGetRelPath( { "hello" } ) != nullptr;
}


/* -------------------------------------------------------------------------- */

  bool
test_FlakePackageSet_iterator1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet ps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  size_t c = 0;
  for ( auto & p : ps ) { ++c; }
  // TODO: Find a way to eval broken packages, then use `fullPkgCount'
  return c == unbrokenPkgCount;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( RawPackageSet_iterator1 );
  RUN_TEST( RawPackageSet_addPackage1 );

  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs(
    inputs
  , prefs
  , (std::list<std::string>) { "x86_64-linux" }
  );

  std::shared_ptr<nix::flake::LockedFlake> flake  =
    rs.getInput( "nixpkgs" ).value()->getLockedFlake();

  /* XXX: This test must go first because it may initialize our DB. */
  RUN_TEST( cachePackageSet1, rs, flake );
  /* XXX: This test must go first because it may initialize our DB. */

  RUN_TEST( DbPackageSet_size1,     flake );
  RUN_TEST( DbPackageSet_iterator1, flake );

  RUN_TEST( FlakePackageSet_hasRelPath1,      rs, flake );
  RUN_TEST( FlakePackageSet_maybeGetRelPath1, rs, flake );
  RUN_TEST( FlakePackageSet_size1,            rs, flake );
  RUN_TEST( FlakePackageSet_iterator1,        rs, flake );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
