/* ========================================================================== *
 *
 * @file flox/core/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>  // For `std::hash' and `std::pair' 
#include <string>      // For `std::string' and `std::string_view'
#include <vector>
#include <list>
#include <optional>
#include <variant>

#include <nix/logging.hh>
#include <nix/store-api.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flakeref.hh>

#include <nlohmann/json.hpp>


/* -------------------------------------------------------------------------- */

/* Backported from C++20a for C++20b compatability. */

/** 
 * @brief Helper type for `std::visit( overloaded { ... }, x );` pattern.
 * 
 * This is a _quality of life_ helper that shortens boilerplate required for
 * creating type matching statements.
 */
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts> overloaded( Ts... ) -> overloaded<Ts...>;


/* -------------------------------------------------------------------------- */

/* Extension to the `nlohmann::json' serializer to support addition STLs. */
namespace nlohmann {

  /** Serializers for `std::optional` */
    template <typename T>
  struct adl_serializer<std::optional<T>> {

      static void
    to_json( json & jto, const std::optional<T> & opt )
    {
      if ( opt.has_value() ) { jto = * opt; }
      else                   { jto = nullptr; }
    }

      static void
    from_json( const json & jfrom, std::optional<T> & opt )
    {
      if ( jfrom.is_null() ) { opt = std::nullopt; }
      else { opt = std::make_optional( jfrom.template get<T>() ); }
    }

  };  /* End struct `adl_serializer<std::optional<T>>' */


    template <typename T>
  struct adl_serializer<std::vector<std::optional<T>>> {

      static void
    to_json( json & jto, const std::vector<std::optional<T>> & vec )
    {
      jto = json::array();
      for ( const auto & opt : vec )
        {
          if ( opt.has_value() ) { jto.push_back( * opt ); }
          else                   { jto.push_back( nullptr ); }
        }
    }

      static void
    from_json( const json & jfrom, std::vector<std::optional<T>> & vec )
    {
      vec.clear();
      for ( const auto & value : jfrom )
        {
          if ( value.is_null() )
            {
              vec.push_back( std::nullopt );
            }
          else
            {
              vec.push_back( std::make_optional( value.template get<T>() ) );
            }
        }
    }

  };  /* End struct `adl_serializer<std::vector<std::optional<T>>>' */


  /* Flake Refs */
    template<>
  struct adl_serializer<nix::FlakeRef> {

      static void
    to_json( json & jto, const nix::FlakeRef & ref )
    {
      jto = nix::fetchers::attrsToJSON( ref.toAttrs() );
    }

    /** _Move-only_ constructor for flake-ref from JSON. */
      static nix::FlakeRef
    from_json( const json & jfrom )
    {
      if ( jfrom.is_object() )
        {
          return {
            nix::FlakeRef::fromAttrs( nix::fetchers::jsonToAttrs( jfrom ) )
          };
        }
      else
        {
          return { nix::parseFlakeRef( jfrom.get<std::string>() ) };
        }
    }

  };  /* End struct `adl_serializer<nix::FlakeRef>' */

  /* Eithers */
    template<typename A, typename B>
  struct adl_serializer<std::variant<A, B>> {

      static void
    to_json( json & jto, const std::variant<A, B> & var )
    {
      if ( std::holds_alternative<A>( var ) )
        {
          jto = std::get<A>( var );
        }
      else
        {
          jto = std::get<B>( var );
        }
    }

      static void
    from_json( const json & jfrom, std::variant<A, B> & var )
    {
      try
        {
          var = jfrom.template get<A>();
        }
      catch( ... )
        {
          var = jfrom.template get<B>();
        }
    }

  };  /* End struct `adl_serializer<std::variant<A, B>>' */


  /* nix::fetchers::Attrs */
    template<>
  struct adl_serializer<nix::fetchers::Attrs> {

      static void
    to_json( json & jto, const nix::fetchers::Attrs & attrs )
    {
      jto = nix::fetchers::attrsToJSON( attrs );
    }

      static void
    from_json( const json & jfrom, nix::fetchers::Attrs & attrs )
    {
      attrs = nix::fetchers::jsonToAttrs( jfrom );
    }

  };  /* End struct `adl_serializer<std::variant<A, B>>' */

}  /* End namespace `nlohmann' */


/* ========================================================================== */

namespace flox {

/* -------------------------------------------------------------------------- */

/** @brief Systems to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultSystems()
{
  static const std::vector<std::string> defaultSystems = {
   "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
  };
  return defaultSystems;
}


/** @brief `flake' subtrees to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultSubtrees()
{
  static const std::vector<std::string> defaultSubtrees = {
    "catalog", "packages", "legacyPackages"
  };
  return defaultSubtrees;
}


/** @brief Catalog stabilities to resolve/search in. */
  inline static const std::vector<std::string> &
getDefaultCatalogStabilities()
{
  static const std::vector<std::string> defaultCatalogStabilities = {
    "stable", "staging", "unstable"
  };
  return defaultCatalogStabilities;
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Detect if a path is a SQLite3 database file.
 * @param dbPath Absolute path.
 * @return `true` iff @a path is a SQLite3 database file.
 */
bool isSQLiteDb( const std::string & dbPath );


/* -------------------------------------------------------------------------- */

/**
 * @brief Parse a flake reference from either a JSON attrset or URI string.
 * @param flakeRef JSON or URI string representing a `nix` flake reference.
 * @return Parsed flake reference object.
 */
  static inline nix::FlakeRef
parseFlakeRef( const std::string & flakeRef )
{
  return ( flakeRef.find( '{' ) == std::string::npos )
         ? nix::parseFlakeRef( flakeRef )
         : nix::FlakeRef::fromAttrs(
             nix::fetchers::jsonToAttrs( nlohmann::json::parse( flakeRef ) )
           );
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Parse a JSON object from an inline string or a path to a JSON file.
 * @param jsonOrPath A JSON string or a path to a JSON file.
 * @return A parsed JSON object.
 */
nlohmann::json parseOrReadJSONObject( const std::string & jsonOrPath );


/* -------------------------------------------------------------------------- */

/** @brief Convert a TOML string to JSON. */
nlohmann::json tomlToJSON( std::string_view toml );


/* -------------------------------------------------------------------------- */

/** @brief Convert a YAML string to JSON. */
nlohmann::json yamlToJSON( std::string_view yaml );


/* -------------------------------------------------------------------------- */

/**
 * @brief Split an attribute path string.
 *
 * Handles quoted strings and escapes.
 */
std::vector<std::string> splitAttrPath( std::string_view path );


/* -------------------------------------------------------------------------- */

/**
 * @brief Is the string @str a positive natural number?
 * @param str String to test.
 * @return `true` iff @a str is a stringized unsigned integer.
 */
bool isUInt( std::string_view str );


/* -------------------------------------------------------------------------- */

/**
 * @brief Does the string @str have the prefix @a prefix?
 * @param prefix The prefix to check for.
 * @param str String to test.
 * @return `true` iff @a str has the prefix @a prefix.
 */
bool hasPrefix( std::string_view prefix, std::string_view str );


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
