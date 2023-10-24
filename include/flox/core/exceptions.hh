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
  EC_INVALID_ARG,
  /** A package descriptor in a manifest is invalid. */
  EC_INVALID_MANIFEST_DESCRIPTOR,
  /** A PkgDescriptorRaw is invalid. */
  EC_INVALID_PKG_DESCRIPTOR,
  /** Errors concerning validity of package query parameters. */
  EC_INVALID_PKG_QUERY_ARG,
  /** A registry has invalid contents. */
  EC_INVALID_REGISTRY,
  /** The value of `manifestPath' is invalid. */
  EC_INVALID_MANIFEST_FILE,
  /** `nix::Error` that doesn't fall under a more specific EC_NIX_* category. */
  EC_NIX,
  /** `nix::EvalError` */
  EC_NIX_EVAL,
  /** Exception locking a flake. */
  EC_NIX_LOCK_FLAKE,
  /** Exception initializing a `FlakePackage`. */
  EC_PACKAGE_INIT,
  /** Exception parsing `QueryParams` from JSON. */
  EC_PARSE_QUERY_PARAMS,
  /** Exception parsing `QueryPreferences` from JSON. */
  EC_PARSE_QUERY_PREFERENCES,
  /** Exception parsing `SearchQuery` from JSON. */
  EC_PARSE_SEARCH_QUERY,
  /** For generic exceptions thrown by `flox::pkgdb::*` classes. */
  EC_PKG_DB,
  /** Exception converting TOML to JSON. */
  EC_TOML_TO_JSON,
  /** Exception converting YAML to JSON. */
  EC_YAML_TO_JSON,

}; /* End enum `error_category' */


/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception
{
private:

  /** Corresponds to an error_category, in this case `EC_FLOX_EXCEPTION`. */
  static constexpr std::string_view categoryMsg = "general error";

  /** Additional context added when the error is thrown. */
  std::optional<std::string> contextMsg;

  /**
   * If some other exception was caught before throwing this one, caughtMsg
   * contains what() of that exception.
   */
  std::optional<std::string> caughtMsg;

public:

  explicit FloxException( std::string_view contextMsg )
    : contextMsg( contextMsg )
  {}

  explicit FloxException( std::string_view contextMsg, const char *caughtMsg )
    : contextMsg( contextMsg ), caughtMsg( caughtMsg )
  {}

  [[nodiscard]] virtual error_category
  getErrorCode() const noexcept
  {
    return EC_FLOX_EXCEPTION;
  }

  [[nodiscard]] virtual std::string_view
  getCategoryMessage() const noexcept
  {
    return this->categoryMsg;
  }

  /**
   * We can't override what() properly because we'd need to dynamically call
   * virtual methods.
   * - We don't want to force overriding what() in every child class, because
   *   that would be a pain.
   * - We can't do that when calling what(), because what() returns a char *
   *   and is const, we don't have anywhere to store that dynamic information.
   * - We can't do it at construction time, because we can't call virtual
   *   methods.
   */
  [[nodiscard]] std::string
  whatString() const noexcept;

  friend void
  to_json( nlohmann::json &jto, const FloxException &err );

}; /* End class `FloxException' */

/* -------------------------------------------------------------------------- */

/** @brief A `nix::EvalError` was encountered.  */
class NixEvalException : public FloxException
{

private:

  static constexpr std::string_view categoryMsg = "invalid argument";

public:

  explicit NixEvalException( std::string_view      contextMsg,
                             const nix::EvalError &err )
    : FloxException( contextMsg,
                     nix::filterANSIEscapes( err.what(), true ).c_str() )
  {}

  [[nodiscard]] error_category
  getErrorCode() const noexcept override
  {
    return EC_NIX_EVAL;
  }

  [[nodiscard]] std::string_view
  getCategoryMessage() const noexcept override
  {
    return this->categoryMsg;
  }

}; /* End class `NixEvalException' */

/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
