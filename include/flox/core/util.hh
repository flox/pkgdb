/* ========================================================================== *
 *
 * @file flox/core/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <functional>  // For `std::hash' and `std::pair'
#include <list>
#include <optional>
#include <string>  // For `std::string' and `std::string_view'
#include <variant>
#include <vector>

#include <nix/eval-cache.hh>
#include <nix/flake/flakeref.hh>
#include <nix/logging.hh>
#include <nix/store-api.hh>

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
struct overloaded : Ts...
{
  using Ts::operator()...;
};

template<class... Ts>
overloaded( Ts... ) -> overloaded<Ts...>;


/* -------------------------------------------------------------------------- */

/**
 * @brief Extension to the `nlohmann::json' serializer to support additional
 *        _Argument Dependent Lookup_ (ADL) types.
 */
namespace nlohmann {

/* -------------------------------------------------------------------------- */

  /** @brief Variants ( Eithers ) of two elements to/from JSON. */
  template<typename A, typename B>
  struct adl_serializer<std::variant<A, B>> {

    /** @brief Convert a @a std::variant<A, B> to a JSON type. */
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

    /** @brief Convert a JSON type to a @a std::variant<A, B>. */
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

/* -------------------------------------------------------------------------- */

  /** @brief Variants ( Eithers ) of three elements to/from JSON. */
  template<typename A, typename B, typename C>
  struct adl_serializer<std::variant<A, B, C>> {

    /** @brief Convert a @a std::variant<A, B, C> to a JSON type. */
      static void
    to_json( json & jto, const std::variant<A, B, C> & var )
    {
      if ( std::holds_alternative<A>( var ) )
        {
          jto = std::get<A>( var );
        }
      else if ( std::holds_alternative<B>( var ) )
        {
          jto = std::get<B>( var );
        }
      else
        {
          jto = std::get<C>( var );
        }
    }

    /** @brief Convert a JSON type to a @a std::variant<A, B, C>. */
      static void
    from_json( const json & jfrom, std::variant<A, B, C> & var )
    {
      try
        {
          var = jfrom.template get<A>();
        }
      catch( ... )
        {
          try {
              var = jfrom.template get<B>();
          }
          catch (...) {
              var = jfrom.template get<C>();
          }
        }
    }

  };  /* End struct `adl_serializer<std::variant<A, B, C>>' */


/* -------------------------------------------------------------------------- */

/** @brief @a nix::fetchers::Attrs to/from JSON */
template<>
struct adl_serializer<nix::fetchers::Attrs>
{

  /** @brief Convert a @a nix::fetchers::Attrs to a JSON object. */
  static void
  to_json( json &jto, const nix::fetchers::Attrs &attrs )
  {
    /* That was easy. */
    jto = nix::fetchers::attrsToJSON( attrs );
  }

  /** @brief Convert a JSON object to a @a nix::fetchers::Attrs. */
  static void
  from_json( const json &jfrom, nix::fetchers::Attrs &attrs )
  {
    /* That was easy. */
    attrs = nix::fetchers::jsonToAttrs( jfrom );
  }

}; /* End struct `adl_serializer<nix::fetchers::Attrs>' */


/* -------------------------------------------------------------------------- */

/** @brief @a nix::FlakeRef to/from JSON. */
template<>
struct adl_serializer<nix::FlakeRef>
{

  /** @brief Convert a @a nix::FlakeRef to a JSON object. */
  static void
  to_json( json &jto, const nix::FlakeRef &ref )
  {
    /* That was easy. */
    jto = nix::fetchers::attrsToJSON( ref.toAttrs() );
  }

  /** @brief _Move-only_ conversion of a JSON object to a @a nix::FlakeRef. */
  static nix::FlakeRef
  from_json( const json &jfrom )
  {
    if ( jfrom.is_object() )
      {
        return { nix::FlakeRef::fromAttrs(
          nix::fetchers::jsonToAttrs( jfrom ) ) };
      }
    else { return { nix::parseFlakeRef( jfrom.get<std::string>() ) }; }
  }

}; /* End struct `adl_serializer<nix::FlakeRef>' */


/* -------------------------------------------------------------------------- */

}  // namespace nlohmann


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** @brief Systems to resolve/search in. */
inline static const std::vector<std::string> &
getDefaultSystems()
{
  static const std::vector<std::string> defaultSystems
    = { "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin" };
  return defaultSystems;
}


/** @brief `flake' subtrees to resolve/search in. */
inline static const std::vector<std::string> &
getDefaultSubtrees()
{
  static const std::vector<std::string> defaultSubtrees
    = { "packages", "legacyPackages" };
  return defaultSubtrees;
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Detect if a path is a SQLite3 database file.
 * @param dbPath Absolute path.
 * @return `true` iff @a path is a SQLite3 database file.
 */
bool
isSQLiteDb( const std::string &dbPath );


/* -------------------------------------------------------------------------- */

/**
 * @brief Parse a flake reference from either a JSON attrset or URI string.
 * @param flakeRef JSON or URI string representing a `nix` flake reference.
 * @return Parsed flake reference object.
 */
static inline nix::FlakeRef
parseFlakeRef( const std::string &flakeRef )
{
  return ( flakeRef.find( '{' ) == std::string::npos )
           ? nix::parseFlakeRef( flakeRef )
           : nix::FlakeRef::fromAttrs(
             nix::fetchers::jsonToAttrs( nlohmann::json::parse( flakeRef ) ) );
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Parse a JSON object from an inline string or a path to a JSON file.
 * @param jsonOrPath A JSON string or a path to a JSON file.
 * @return A parsed JSON object.
 */
nlohmann::json
parseOrReadJSONObject( const std::string &jsonOrPath );


/* -------------------------------------------------------------------------- */

/** @brief Convert a TOML string to JSON. */
nlohmann::json
tomlToJSON( std::string_view toml );


/* -------------------------------------------------------------------------- */

/** @brief Convert a YAML string to JSON. */
nlohmann::json
yamlToJSON( std::string_view yaml );


/* -------------------------------------------------------------------------- */

/**
 * @brief Read a file and coerce its contents to JSON based on its extension.
 *
 * Files with the extension `.json` are parsed directly.
 * Files with the extension `.yaml` or `.yml` are converted to JSON from YAML.
 * Files with the extension `.toml` are converted to JSON from TOML.
 */
nlohmann::json
readAndCoerceJSON( const std::filesystem::path &path );


/* -------------------------------------------------------------------------- */

/**
 * @brief Split an attribute path string.
 *
 * Handles quoted strings and escapes.
 */
std::vector<std::string>
splitAttrPath( std::string_view path );


/* -------------------------------------------------------------------------- */

/**
 * @brief Is the string @str a positive natural number?
 * @param str String to test.
 * @return `true` iff @a str is a stringized unsigned integer.
 */
bool
isUInt( std::string_view str );


/* -------------------------------------------------------------------------- */

/**
 * @brief Does the string @str have the prefix @a prefix?
 * @param prefix The prefix to check for.
 * @param str String to test.
 * @return `true` iff @a str has the prefix @a prefix.
 */
bool
hasPrefix( std::string_view prefix, std::string_view str );


/* -------------------------------------------------------------------------- */

/** @brief trim from start ( in place ). */
std::string &
ltrim( std::string &str );

/** @brief trim from end ( in place ). */
std::string &
rtrim( std::string &str );

/** @brief trim from both ends ( in place ). */
std::string &
trim( std::string &str );


/** @brief trim from start ( copying ). */
[[nodiscard]] std::string
ltrim_copy( std::string_view str );

/** @brief trim from end ( copying ). */
[[nodiscard]] std::string
rtrim_copy( std::string_view str );

/** @brief trim from both ends ( copying ). */
[[nodiscard]] std::string
trim_copy( std::string_view str );


/* -------------------------------------------------------------------------- */

/**
 * @brief Extract the user-friendly portion of a @a nlohmann::json::exception.
 *
 */
std::string
extract_json_errmsg( nlohmann::json::exception &e );

/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
