/* ========================================================================== *
 *
 * @file get.cc
 *
 * @brief Implementation of `pkgdb get` subcommand.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include <nlohmann/json.hpp>

#include "pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

GetCommand::GetCommand()
  : flox::NixState()
  , parser( "get" )
  , pId( "id" )
  , pPath( "path" )
  , pFlake( "flake" )
  , pDb( "db" )
{
  this->parser.add_description( "Get metadata from Package DB" );

  this->pId.add_description( "Lookup an attribute set or package row `id`" );
  this->pId.add_argument( "-p", "--pkg" )
             .help( "Lookup package path" )
             .nargs( 0 )
             .action( [&]( const auto & ) { this->isPkg = true; } );
  this->addTargetArg( this->pId );
  this->addAttrPathArgs( this->pId );
  this->parser.add_subparser( this->pId );

  this->pPath.add_description(
    "Lookup an (AttrSets|Packages).id attribute path"
  );
  this->pPath.add_argument( "-p", "--pkg" )
             .help( "Lookup `Packages.id'" )
             .nargs( 0 )
             .action( [&]( const auto & ) { this->isPkg = true; } );
  this->addTargetArg( this->pPath );
  this->pPath.add_argument( "id" )
             .help( "Row `id' to lookup" )
             .nargs( 1 )
             .action( [&]( const std::string & i )
                      {
                        this->id = std::stoull( i );
                      }
                    );
  this->parser.add_subparser( this->pPath );

  this->pFlake.add_description( "Get flake metadata from Package DB" );
  this->addTargetArg( this->pFlake );
  this->parser.add_subparser( this->pFlake );

  this->pDb.add_description( "Get absolute path to Package DB for a flake" );
  this->addTargetArg( this->pDb );
  this->parser.add_subparser( this->pDb );
}


/* -------------------------------------------------------------------------- */

  int
GetCommand::runId()
{
  if ( this->isPkg )
    {
      std::cout << this->db->getPackageId( this->attrPath ) << std::endl;
    }
  else
    {
      std::cout << this->db->getAttrSetId( this->attrPath ) << std::endl;
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

  int
GetCommand::runPath()
{
  if ( this->isPkg )
    {
      std::cout << nlohmann::json( this->db->getPackagePath( this->id ) ).dump()
                << std::endl;
    }
  else
    {
      std::cout << nlohmann::json( this->db->getAttrSetPath( this->id ) ).dump()
                << std::endl;
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

  int
GetCommand::runFlake()
{
  nlohmann::json j = {
    { "string",      this->db->lockedRef.string                            }
  , { "attrs",        this->db->lockedRef.attrs                            }
  , { "fingerprint", this->db->fingerprint.to_string( nix::Base16, false ) }
  };
  std::cout << j.dump() << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

  int
GetCommand::runDb()
{
  std::cout << ( (std::string) this->dbPath.value() ) << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

  int
GetCommand::run()
{
  if ( this->parser.is_subcommand_used( "id" ) )
    {
      return this->runId();
    }
  else if ( this->parser.is_subcommand_used( "path" ) )
    {
      return this->runPath();
    }
  else if ( this->parser.is_subcommand_used( "flake" ) )
    {
      return this->runFlake();
    }
  else if ( this->parser.is_subcommand_used( "db" ) )
    {
      return this->runDb();
    }
  else
    {
      std::cerr << this->parser << std::endl;
      throw flox::FloxException(
              "You must provide a valid 'get' subcommand"
            );
      return EXIT_FAILURE;
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
