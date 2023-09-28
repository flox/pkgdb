/* ========================================================================== *
 *
 * @file tests/search-params.cc
 *
 * @brief Minimal executable that parses a @a flox::search::SearchParams struct.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <cstdlib>
#include <algorithm>

#include <nlohmann/json.hpp>

#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

/**
 * @brief Ensure defaults/fallbacks work correctly with
 *        @a flox::search::SearchParams `from_json`.
 */
  bool
test_SearchParams_defaults0() {

  return true;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{

  if ( argc < 2 )
    {
      std::cerr << "ERROR: You must provide a JSON string as the "
                << "first argument." << std::endl;
      return EXIT_FAILURE;
    }


  /* Parse */
  nlohmann::json paramsJSON;
  try
    {
      paramsJSON = flox::parseOrReadJSONObject( argv[1] );
    }
  catch( const std::exception & err )
    {
      std::cerr << "ERROR: Failed to parse search parameters: "
                << err.what() << std::endl;
      return EXIT_FAILURE + 1;
    }
  catch( ... )
    {
      std::cerr << "ERROR: Failed to parse search parameters." << std::endl;
      return EXIT_FAILURE + 2;
    }


  /* Deserialize */
  flox::search::SearchParams params;
  try
    {
      paramsJSON.get_to( params );
    }
  catch( const std::exception & err )
    {
      std::cerr << "ERROR: Failed to convert search parameters from JSON: "
                << err.what() << std::endl;
      return EXIT_FAILURE + 3;
    }
  catch( ... )
    {
      std::cerr << "ERROR: Failed to convert search parameters from JSON."
                << std::endl;
      return EXIT_FAILURE + 4;
    }


  /* Serialize */
  try
    {
      std::cout << nlohmann::json( params ).dump() << std::endl;
    }
  catch( const std::exception & err )
    {
      std::cerr << "ERROR: Failed to serialize search parameters: "
                << err.what() << std::endl;
      return EXIT_FAILURE + 5;
    }
  catch( ... )
    {
      std::cerr << "ERROR: Failed to serialize search parameters." << std::endl;
      return EXIT_FAILURE + 6;
    }

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
