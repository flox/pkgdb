/* ========================================================================== *
 *
 * @file tests/id2attrpath.cc
 *
 * @brief Minimal executable to return the attribute path given an attrset id.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <string>
#include <cstdlib>

#include "flox/util.hh"
#include "pkgdb.hh"


/* -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
  if (argc != 2)
    {
      std::cout << "usage: id2attrpath SOURCE ID" << std::endl;
      return EXIT_FAILURE;
    }

  std::string source( argv[1] );
  std::string idStr( argv[2] );

  /* Attempt to parse the id to an int */
  flox::pkgdb::row_id id;
  try
    {
      id = std::stoull( idStr );
    }
  catch( const std::exception & e )
    {
      std::cerr << "Couldn't convert 'id' to 'int'" << std::endl;
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  
  /* Act on the source (database vs. flake ref) */
  if ( flox::isSQLiteDb( source ) )
    {
      flox::pkgdb::PkgDb db = flox::pkgdb::PkgDb( source );
      flox::AttrPath attrPath;
      /* The user-provided id may not be in the database */
      try
        {
          attrPath = db.getAttrSetPath( id );
        }
      catch ( const std::exception & e )
        {
          std::cerr << "Failed to retrieve attrpath with id " << id << std::endl;
          std::cerr << e.what() << std::endl;
          return EXIT_FAILURE;
        }
      
      for (size_t i = 0; i < attrPath.size() - 1; i++)
        {
          std::cout << attrPath[i] << " ";
        }
      std::cout << attrPath[attrPath.size() - 1] << std::endl;
    }
  else
    {
      std::cout << "flake references aren't a supported source yet"
                << std::endl;
      return EXIT_FAILURE;
    }
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
