/* ========================================================================== *
 *
 * @file command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/command.hh"
#include "flox/util.hh"
#include "pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace command {

/* -------------------------------------------------------------------------- */

VerboseParser::VerboseParser( std::string name, std::string version )
  : argparse::ArgumentParser( name, version, argparse::default_arguments::help )
{
    this->add_argument( "-q", "--quiet" )
      .help(
        "Decreate the logging verbosity level. May be used up to 3 times."
      ).action(
        [&]( const auto & )
        {
          nix::verbosity =
            ( nix::verbosity <= nix::lvlError )
              ? nix::lvlError : (nix::Verbosity) ( nix::verbosity - 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append();

    this->add_argument( "-v", "--verbose" )
      .help( "Increase the logging verbosity level. May be used up to 4 times." )
      .action(
        [&]( const auto & )
        {
          nix::verbosity =
            ( nix::lvlVomit <= nix::verbosity )
              ? nix::lvlVomit : (nix::Verbosity) ( nix::verbosity + 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append();
}


/* -------------------------------------------------------------------------- */

  void
FloxFlakeMixin::parseFloxFlake( const std::string & flakeRef )
{
  nix::FlakeRef ref = flox::parseFlakeRef( flakeRef );
  {
    nix::Activity act(
      * nix::logger
    , nix::lvlInfo
    , nix::actUnknown
    , nix::fmt( "fetching flake '%s'", ref.to_string() )
    );
    this->flake = std::make_unique<flox::FloxFlake>(
      (nix::ref<nix::EvalState>) this->state
    , ref
    );
  }

  if ( ! this->flake->lockedFlake.flake.lockedRef.input.hasAllInfo()
     )
    {
      if ( nix::lvlWarn <= nix::verbosity )
        {
          nix::logger->warn(
            "flake-reference is unlocked/dirty - "
            "resulting DB may not be cached."
          );
        }
    }
}


  argparse::Argument &
FloxFlakeMixin::addFlakeRefArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "flake-ref" )
               .help(
                 "flake-ref URI string or JSON attrs ( preferably locked )"
               )
               .required()
               .metavar( "FLAKE-REF" )
               .action( [&]( const std::string & ref )
                        {
                          this->parseFloxFlake( ref );
                        }
                      );
}


/* -------------------------------------------------------------------------- */

  argparse::Argument &
DbPathMixin::addDatabasePathOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "-d", "--database" )
               .help( "Use database at PATH" )
               .default_value( "" )
               .metavar( "PATH" )
               .nargs( 1 )
               .action( [&]( const std::string & dbPath )
                        {
                          this->dbPath = nix::absPath( dbPath );
                          std::filesystem::create_directories(
                            this->dbPath.value().parent_path()
                          );
                        }
                      );
}


/* -------------------------------------------------------------------------- */

  void
PkgDbMixin::openPkgDb()
{
  if ( this->db != nullptr ) { return; }  /* Already loaded. */
  if ( ( this->flake != nullptr ) && this->dbPath.has_value() )
    {
      this->db = std::make_unique<flox::pkgdb::PkgDb>(
        this->flake->lockedFlake
      , (std::string) this->dbPath.value()
      );
    }
  else if ( this->flake != nullptr )
    {
      this->dbPath =
        flox::pkgdb::genPkgDbName( this->flake->lockedFlake );
      this->db = std::make_unique<flox::pkgdb::PkgDb>(
        this->flake->lockedFlake
      , (std::string) this->dbPath.value()
      );
    }
  else if ( this->dbPath.has_value() )
    {
      this->db = std::make_unique<flox::pkgdb::PkgDb>(
        (std::string) this->dbPath.value()
      );
    }
  else
    {
      throw flox::FloxException(
        "You must provide either a path to a database, or a flake-reference."
      );
    }
}


  argparse::Argument &
PkgDbMixin::addTargetArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "target" )
               .help( "The source ( database path or flake-ref ) to read" )
               .required()
               .metavar( "DB-OR-FLAKE-REF" )
               .action(
                 [&]( const std::string & target )
                 {
                   if ( flox::isSQLiteDb( target ) )
                     {
                       this->dbPath = nix::absPath( target );
                     }
                   else  /* flake-ref */
                     {
                       this->parseFloxFlake( target );
                     }
                   this->openPkgDb();
                 }
               );
}


/* -------------------------------------------------------------------------- */

  argparse::Argument &
AttrPathMixin::addAttrPathArgs( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "attr-path" )
               .help( "Attribute path to scrape" )
               .metavar( "ATTRS..." )
               .remaining()
               .action( [&]( const std::string & p )
                        {
                          this->attrPath.emplace_back( p );
                        }
                      );
}


  void
AttrPathMixin::postProcessArgs()
{
  if ( this->attrPath.size() < 1 ) { this->attrPath.push_back( "packages" ); }
  if ( this->attrPath.size() < 2 )
    {
      this->attrPath.push_back(
        nix::settings.thisSystem.get()
      );
    }
  if ( ( this->attrPath.size() < 3 ) &&
       ( this->attrPath[0] == "catalog" )
     )
    {
      this->attrPath.push_back( "stable" );
    }
}


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
