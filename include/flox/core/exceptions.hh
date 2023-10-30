/* ========================================================================== *
 *
 * @file flox/core/exceptions.hh
 *
 * @brief Definitions of various `std::exception` children used for throwing
 *        errors with nice messages and typed discrimination.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nix/nixexpr.hh>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

enum error_category {
  /** Indicates success or _not an error_. */
  EC_OKAY = 0,
  /**
   * Returned for any exception that doesn't have `getErrorCode()`, i.e.
   * exceptions we haven't wrapped in a custom exception.
   */
  EC_FAILURE = 1,
  /** Generic exception emitted by `flox` routines. */
  EC_FLOX_EXCEPTION = 100,
  /** A command line argument is invalid. */
  EC_INVALID_ARG = 101,
  /** A package descriptor in a manifest is invalid. */
  EC_INVALID_MANIFEST_DESCRIPTOR = 102,
  /** A PkgDescriptorRaw is invalid. */
  EC_INVALID_PKG_DESCRIPTOR = 103,
  /** Errors concerning validity of package query parameters. */
  EC_INVALID_PKG_QUERY_ARG = 104,
  /** A registry has invalid contents. */
  EC_INVALID_REGISTRY = 105,
  /** The value of `manifestPath' is invalid. */
  EC_INVALID_MANIFEST_FILE = 106,
  /**
   * `nix::Error` that doesn't fall under a more specific `EC_NIX_*` category.
   */
  EC_NIX = 107,
  /** `nix::EvalError` */
  EC_NIX_EVAL = 108,
  /** Exception locking a flake. */
  EC_NIX_LOCK_FLAKE = 109,
  /** Exception initializing a @a flox::FlakePackage. */
  EC_PACKAGE_INIT = 110,
  /** Exception parsing @a flox::pkgdb::QueryParams from JSON. */
  EC_PARSE_QUERY_PARAMS = 111,
  /** Exception parsing @a flox::pkgdb::QueryPreferences from JSON. */
  EC_PARSE_QUERY_PREFERENCES = 112,
  /** Exception parsing @a flox::search::SearchQuery from JSON. */
  EC_PARSE_SEARCH_QUERY = 113,
  /** For generic exceptions thrown by `flox::pkgdb::*` classes. */
  EC_PKG_DB = 114,
  /** Exceptions thrown by SQLite3. */
  EC_SQLITE3 = 115,
  /** Exception parsing/processing JSON. */
  EC_JSON = 116,
  /** Exception converting TOML to JSON. */
  EC_TOML_TO_JSON = 117,
  /** Exception converting YAML to JSON. */
  EC_YAML_TO_JSON = 118

}; /* End enum `error_category' */


/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception
{

private:

  /** Additional context added when the error is thrown. */
  std::optional<std::string> contextMsg;

  /**
   * If some other exception was caught before throwing this one, @a caughtMsg
   * contains what() of that exception.
   */
  std::optional<std::string> caughtMsg;

  /** The final what() message. */
  std::string whatMsg;


public:

  /**
   * @brief Create a generic exception with a custom message.
   *
   * This constructor is NOT suitable for use by _child classes_.
   */
  explicit FloxException( std::string_view contextMsg )
    : contextMsg( contextMsg )
    , whatMsg( "general error: " + std::string( contextMsg ) )
  {}

  /**
   * @brief Create a generic exception with a custom message and information
   *        from a child error.
   *
   * This constructor is NOT suitable for use by _child classes_.
   */
  explicit FloxException( std::string_view contextMsg,
                          std::string_view caughtMsg )
    : contextMsg( contextMsg )
    , caughtMsg( caughtMsg )
    , whatMsg( "general error: " + std::string( contextMsg ) + ": "
               + std::string( caughtMsg ) )
  {}

  /**
   * @brief Directly initialize a FloxException with a custom category message,
   *        (optional) _context_, and (optional) information from a child error.
   *
   * This form is recommended for use by _child classes_ which
   * extend @a flox::FloxException.
   *
   * @see FLOX_DEFINE_EXCEPTION
   */
  explicit FloxException( std::string_view           categoryMsg,
                          std::optional<std::string> contextMsg,
                          std::optional<std::string> caughtMsg )
    : contextMsg( contextMsg ), caughtMsg( caughtMsg ), whatMsg( categoryMsg )
  {
    /* Finish initializing `categoryMsg` if we received `contextMsg` or
     * `caughtMsg` values. */
    if ( contextMsg.has_value() ) { this->whatMsg += ": " + ( *contextMsg ); }
    if ( caughtMsg.has_value() ) { this->whatMsg += ": " + ( *caughtMsg ); }
  }


  [[nodiscard]] virtual error_category
  getErrorCode() const noexcept
  {
    return EC_FLOX_EXCEPTION;
  }

  [[nodiscard]] std::optional<std::string>
  getContextMessage() const noexcept
  {
    return this->contextMsg;
  }

  [[nodiscard]] std::optional<std::string>
  getCaughtMessage() const noexcept
  {
    return this->caughtMsg;
  }

  [[nodiscard]] virtual std::string_view
  getCategoryMessage() const noexcept
  {
    return "general error";
  }

  /** @brief Produces an explanatory string about an exception. */
  [[nodiscard]] const char *
  what() const noexcept override
  {
    return this->whatMsg.c_str();
  }


}; /* End class `FloxException' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a @a flox::FloxException to a JSON object. */
void
to_json( nlohmann::json &jto, const FloxException &err );


/* -------------------------------------------------------------------------- */

// NOLINTBEGIN(bugprone-macro-parentheses)
//  Disable macro parentheses lint so we can use `NAME' symbol directly.

/**
 * @brief Generate a class definition with an error code and
 *        _category message_.
 *
 * The resulting class will have `NAME()`, `NAME( contextMsg )`,
 * and `NAME( contextMsg, caughtMsg )` constructors available.
 */
#define FLOX_DEFINE_EXCEPTION( NAME, ERROR_CODE, CATEGORY_MSG )              \
  class NAME : public FloxException                                          \
  {                                                                          \
  public:                                                                    \
                                                                             \
    NAME() : FloxException( CATEGORY_MSG, std::nullopt, std::nullopt ) {}    \
                                                                             \
    explicit NAME( std::string_view contextMsg )                             \
      : FloxException( ( CATEGORY_MSG ),                                     \
                       std::string( contextMsg ),                            \
                       std::nullopt )                                        \
    {}                                                                       \
                                                                             \
    explicit NAME( std::string_view contextMsg, std::string_view caughtMsg ) \
      : FloxException( ( CATEGORY_MSG ),                                     \
                       std::string( contextMsg ),                            \
                       std::string( caughtMsg ) )                            \
    {}                                                                       \
                                                                             \
    [[nodiscard]] error_category                                             \
    getErrorCode() const noexcept override                                   \
    {                                                                        \
      return ( ERROR_CODE );                                                 \
    }                                                                        \
                                                                             \
    [[nodiscard]] std::string_view                                           \
    getCategoryMessage() const noexcept override                             \
    {                                                                        \
      return ( CATEGORY_MSG );                                               \
    }                                                                        \
  };
// NOLINTEND(bugprone-macro-parentheses)


/* -------------------------------------------------------------------------- */

/** @brief A `nix::EvalError` was encountered.  */
class NixEvalException : public FloxException
{

public:

  explicit NixEvalException( std::string_view      contextMsg,
                             const nix::EvalError &err )
    : FloxException( "invalid argument",
                     std::string( contextMsg ),
                     nix::filterANSIEscapes( err.what(), true ) )
  {}

  [[nodiscard]] error_category
  getErrorCode() const noexcept override
  {
    return EC_NIX_EVAL;
  }

  [[nodiscard]] std::string_view
  getCategoryMessage() const noexcept override
  {
    return "Nix evaluation error";
  }


}; /* End class `NixEvalException' */


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
