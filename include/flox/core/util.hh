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

/** @brief Optional types to/from JSON. */
template<typename T>
struct adl_serializer<std::optional<T>>
{

  /**
   *  @brief Convert an optional type to a JSON type  treating `std::nullopt`
   *         as `null`.
   */
  static void
  to_json( json & jto, const std::optional<T> & opt )
  {
    if ( opt.has_value() ) { jto = *opt; }
    else { jto = nullptr; }
  }

  /**
   * @brief Convert a JSON type to an `optional<T>` treating
   *        `null` as `std::nullopt`.
   */
  static void
  from_json( const json & jfrom, std::optional<T> & opt )
  {
    if ( jfrom.is_null() ) { opt = std::nullopt; }
    else { opt = std::make_optional( jfrom.template get<T>() ); }
  }

}; /* End struct `adl_serializer<std::optional<T>>' */


/* -------------------------------------------------------------------------- */

/** @brief Variants ( Eithers ) of two elements to/from JSON. */
template<typename A, typename B>
struct adl_serializer<std::variant<A, B>>
{

  /** @brief Convert a @a std::variant<A, B> to a JSON type. */
  static void
  to_json( json & jto, const std::variant<A, B> & var )
  {
    if ( std::holds_alternative<A>( var ) ) { jto = std::get<A>( var ); }
    else { jto = std::get<B>( var ); }
  }

  /** @brief Convert a JSON type to a @a std::variant<A, B>. */
  static void
  from_json( const json & jfrom, std::variant<A, B> & var )
  {
    try
      {
        var = jfrom.template get<A>();
      }
    catch ( ... )
      {
        var = jfrom.template get<B>();
      }
  }

}; /* End struct `adl_serializer<std::variant<A, B>>' */


/* -------------------------------------------------------------------------- */

/**
 * @brief Variants ( Eithers ) of any number of elements to/from JSON.
 *
 * The order of your types effects priority.
 * Any valid parse or coercion from a type named _early_ in the variant list
 * will succeed before attempting to parse alternatives.
 *
 * For example, always attempt `bool` first, then `int`, then `float`, and
 * alway attempt `std::string` LAST.
 *
 * It's important to note that you must never nest multiple `std::optional`
 * types in a variant, instead make `std::optional<std::variant<...>>`.
 */
template<typename A, typename... Types>
struct adl_serializer<std::variant<A, Types...>>
{

  /** @brief Convert a @a std::variant<A, Types...> to a JSON type. */
  static void
  to_json( json & jto, const std::variant<A, Types...> & var )
  {
    /* This _unwraps_ the inner type and calls the proper `to_json'.
     * The compiler does the heavy lifting for us here <3. */
    std::visit( [&]( auto unwrapped ) -> void { jto = unwrapped; }, var );
  }

  /** @brief Convert a JSON type to a @a std::variant<Types...>. */
  static void
  from_json( const json & jfrom, std::variant<A, Types...> & var )
  {
    /* Try getting typename `A', or recur. */
    try
      {
        var = jfrom.template get<A>();
      }
    catch ( ... )
      {
        /* Strip typename `A' from variant, and call recursively. */
        using next_variant = std::variant<Types...>;

        /* Coerce to `next_variant' type. */
        next_variant next = jfrom.template get<next_variant>();
        std::visit( [&]( auto unwrapped ) -> void { var = unwrapped; }, next );
      }
  }

}; /* End struct `adl_serializer<std::variant<A, Types...>>' */


/* -------------------------------------------------------------------------- */

/** @brief @a nix::fetchers::Attrs to/from JSON */
template<>
struct adl_serializer<nix::fetchers::Attrs>
{

  /** @brief Convert a @a nix::fetchers::Attrs to a JSON object. */
  static void
  to_json( json & jto, const nix::fetchers::Attrs & attrs )
  {
    /* That was easy. */
    jto = nix::fetchers::attrsToJSON( attrs );
  }

  /** @brief Convert a JSON object to a @a nix::fetchers::Attrs. */
  static void
  from_json( const json & jfrom, nix::fetchers::Attrs & attrs )
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
  to_json( json & jto, const nix::FlakeRef & ref )
  {
    /* That was easy. */
    jto = nix::fetchers::attrsToJSON( ref.toAttrs() );
  }

  /** @brief _Move-only_ conversion of a JSON object to a @a nix::FlakeRef. */
  static nix::FlakeRef
  from_json( const json & jfrom )
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
  static const std::vector<std::string> defaultSystems = { "x86_64-linux",
                                                           "aarch64-linux",
                                                           "x86_64-darwin",
                                                           "aarch64-darwin" };
  return defaultSystems;
}


/** @brief `flake' subtrees to resolve/search in. */
inline static const std::vector<std::string> &
getDefaultSubtrees()
{
  static const std::vector<std::string> defaultSubtrees = { "catalog",
                                                            "packages",
                                                            "legacyPackages" };
  return defaultSubtrees;
}


/** @brief Catalog stabilities to resolve/search in. */
inline static const std::vector<std::string> &
getDefaultCatalogStabilities()
{
  static const std::vector<std::string> defaultCatalogStabilities = {
    "stable",
    "staging",
    "unstable"
  };
  return defaultCatalogStabilities;
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Detect if a path is a SQLite3 database file.
 * @param dbPath Absolute path.
 * @return `true` iff @a path is a SQLite3 database file.
 */
bool
isSQLiteDb( const std::string & dbPath );


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
           : nix::FlakeRef::fromAttrs( nix::fetchers::jsonToAttrs(
               nlohmann::json::parse( flakeRef ) ) );
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Parse a JSON object from an inline string or a path to a JSON file.
 * @param jsonOrPath A JSON string or a path to a JSON file.
 * @return A parsed JSON object.
 */
nlohmann::json
parseOrReadJSONObject( const std::string & jsonOrPath );


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
ltrim( std::string & str );

/** @brief trim from end ( in place ). */
std::string &
rtrim( std::string & str );

/** @brief trim from both ends ( in place ). */
std::string &
trim( std::string & str );


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

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
