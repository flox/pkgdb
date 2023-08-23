/* ========================================================================== *
 *
 * @file pkgdb/command.cc
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

#include "flox/pkgdb/command.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

  argparse::Argument &
DbPathMixin::addDatabasePathOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "-d", "--database" )
               .help( "Use database at PATH" )
               .metavar( "PATH" )
               .nargs( 1 )
               .action( [&]( const std::string & dbPath )
                        {
                          this->dbPath = nix::absPath( dbPath );
                          std::filesystem::create_directories(
                            this->dbPath->parent_path()
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
      , (std::string) * this->dbPath
      );
    }
  else if ( this->flake != nullptr )
    {
      this->dbPath = flox::pkgdb::genPkgDbName(
        this->flake->lockedFlake.getFingerprint()
      );
      std::filesystem::create_directories( this->dbPath->parent_path() );
      this->db = std::make_unique<flox::pkgdb::PkgDb>(
        this->flake->lockedFlake
      , (std::string) * this->dbPath
      );
    }
  else if ( this->dbPath.has_value() )
    {
      std::filesystem::create_directories( this->dbPath->parent_path() );
      this->db = std::make_unique<flox::pkgdb::PkgDb>(
        (std::string) * this->dbPath
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

  }  /* End namespaces `flox::pkgdb' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
