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
#include "flox/pkgdb/write.hh"

/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

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

  template <> void
PkgDbMixin<PkgDb>::openPkgDb()
{
  if ( this->db != nullptr ) { return; }  /* Already loaded. */
  if ( ( this->flake != nullptr ) && this->dbPath.has_value() )
    {
      this->db = std::make_shared<PkgDb>(
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
      this->db = std::make_shared<PkgDb>(
        this->flake->lockedFlake
      , (std::string) * this->dbPath
      );
    }
  else if ( this->dbPath.has_value() )
    {
      std::filesystem::create_directories( this->dbPath->parent_path() );
      this->db = std::make_shared<PkgDb>( (std::string) * this->dbPath );
    }
  else
    {
      throw flox::FloxException(
        "You must provide either a path to a database, or a flake-reference."
      );
    }
}


/* -------------------------------------------------------------------------- */

  template <> void
PkgDbMixin<PkgDbReadOnly>::openPkgDb()
{
  if ( this->db != nullptr ) { return; }  /* Already loaded. */

  if ( ! this->dbPath.has_value() )
    {
      if ( this->flake == nullptr )
        {
          throw flox::FloxException(
            "You must provide either a path to a database, or "
            "a flake-reference."
          );
        }
      this->dbPath = flox::pkgdb::genPkgDbName(
        this->flake->lockedFlake.getFingerprint()
      );
    }

  /* Initialize empty DB if none exists. */
  if ( ! std::filesystem::exists( this->dbPath.value() ) )
    {
      if ( this->flake != nullptr )
        {
          std::filesystem::create_directories( this->dbPath->parent_path() );
          flox::pkgdb::PkgDb pdb( this->flake->lockedFlake
                                , (std::string) * this->dbPath
                                );
        }
    }

  if ( this->flake != nullptr )
    {
      this->db = std::make_shared<PkgDbReadOnly>(
        this->flake->lockedFlake.getFingerprint()
      , (std::string) * this->dbPath
      );
    }
  else
    {
      this->db =
        std::make_shared<PkgDbReadOnly>( (std::string) * this->dbPath );
    }
}


/* -------------------------------------------------------------------------- */

  template <PkgDbType T> argparse::Argument &
PkgDbMixin<T>::addTargetArg( argparse::ArgumentParser & parser )
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

template struct PkgDbMixin<PkgDbReadOnly>;
template struct PkgDbMixin<PkgDb>;


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
