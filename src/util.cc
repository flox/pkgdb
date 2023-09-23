/* ========================================================================== *
 *
 * @file flox/util.cc
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include <nlohmann/json.hpp>

#include "flox/core/util.hh"
#include "flox/core/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  bool
isSQLiteDb( const std::string & dbPath )
{
  std::filesystem::path path( dbPath );
  if ( ! std::filesystem::exists( path ) )     { return false; }
  if ( std::filesystem::is_directory( path ) ) { return false; }

  /* Read file magic */
  static const char expectedMagic[16] = "SQLite format 3";  // NOLINT

  char buffer[16];  // NOLINT
  std::memset( & buffer[0], '\0', sizeof( buffer ) );
  FILE * filep = fopen( dbPath.c_str(), "rb" );

  std::clearerr( filep );

  const size_t nread =
    std::fread( & buffer[0], sizeof( buffer[0] ), sizeof( buffer ), filep );
  if ( nread != sizeof( buffer ) )
    {
      if ( std::feof( filep ) != 0 )
        {
          std::fclose( filep );  // NOLINT
          return false;
        }
      if ( std::ferror( filep ) != 0 )
        {
          std::fclose( filep );  // NOLINT
          throw flox::FloxException( "Failed to read file " + dbPath );
        }
      std::fclose( filep );  // NOLINT
      return false;
    }
  std::fclose( filep );  // NOLINT
  return std::string_view( & buffer[0] ) ==
         std::string_view( & expectedMagic[0] );
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
parseOrReadJSONObject( const std::string & jsonOrPath )
{
  if ( jsonOrPath.find( '{' ) != std::string::npos )
    {
      return nlohmann::json::parse( jsonOrPath );
    }
  std::ifstream jfile( jsonOrPath );
  return nlohmann::json::parse( jfile );
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string>
splitAttrPath( std::string_view path )
{
  std::vector<std::string> parts;

  bool inSingleQuote = false;
  bool inDoubleQuote = false;
  bool wasEscaped    = false;
  auto start         = path.begin();

  /* Remove outer quotes and unescape. */
  auto dequote = [&]( const std::string & part ) -> std::string
    {
      auto itr = part.begin();
      auto end = part.end();

      /* Remove outer quotes. */
      if ( ( ( part.front() == '\'' ) && ( part.back() == '\'' ) ) ||
           ( ( part.front() == '"' )  && ( part.back() == '"' ) )
         )
        {
          ++itr;
          --end;
        }

      /* Remove escape characters. */
      std::string rsl;
      bool        wasEscaped = false;
      for ( ; itr != end; ++itr )
        {
          if ( wasEscaped )
            {
              wasEscaped = false;
            }
          else if ( ( * itr ) == '\\' )
            {
              wasEscaped = true;
              continue;
            }
          rsl.push_back( * itr );
        }

      return rsl;
    };  /* End lambda `dequote' */

  /* Split by dots, handling quotes. */
  for ( auto itr = path.begin(); itr != path.end(); ++itr )
    {
      if ( wasEscaped )
        {
          wasEscaped = false;
        }
      else if ( ( * itr ) == '\\' )
        {
          wasEscaped = true;
        }
      else if ( ( ( * itr ) == '\'' ) && ( ! inDoubleQuote ) )
        {
          inSingleQuote = ! inSingleQuote;
        }
      else if ( ( ( * itr ) == '"' ) && ( ! inSingleQuote ) )
        {
          inDoubleQuote = ! inDoubleQuote;
        }
      else if ( * itr == '.' && ( ! inSingleQuote ) && ( ! inDoubleQuote ) )
        {
          parts.emplace_back( dequote( std::string( start, itr ) ) );
          start = itr + 1;
        }
    }

  if ( start != path.end() )
    {
      parts.emplace_back( dequote( std::string( start, path.end() ) ) );
    }

  return parts;
}


/* -------------------------------------------------------------------------- */

  bool
isUInt( std::string_view str )
{
  return ( ! str.empty() ) &&
         ( std::find_if(
             str.begin()
           , str.end()
           , []( unsigned char chr ) { return std::isdigit( chr ) == 0; }
           ) == str.end()
         );
}


/* -------------------------------------------------------------------------- */

  bool
hasPrefix( std::string_view prefix, std::string_view str )
{
  return str.compare( 0, prefix.size(), prefix ) == 0;
}


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
