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

#include <optional>
#include <stdexcept>
#include <string>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

enum error_category {
  EC_OKAY = 0,
  // Returned for any exception that doesn't have an error_code(), i.e.
  // exceptions we haven't wrapped in a custom exception
  EC_FAILURE        = 1,
  EC_FLOX_EXCEPTION = 100,
  // A command line argument is invalid.
  EC_INVALID_ARG = 101,
  // A package descriptor in a manifest is invalid.
  EC_INVALID_MANIFEST_DESCRIPTOR = 102,
  // A PkgDescriptorRaw is invalid.
  EC_INVALID_PKG_DESCRIPTOR = 103,
  // Errors concerning validity of package query parameters
  EC_INVALID_PKG_QUERY_ARG = 104,
  // A registry has invalid contents.
  EC_INVALID_REGISTRY = 105,
  // The value of registryPath is invalid.
  EC_INVALID_REGISTRY_FILE = 106,
  // Exception initializing a `FlakePackage`
  EC_PACKAGE_INIT = 107,
  // Exception parsing `QueryParams` from JSON
  EC_PARSE_QUERY_PARAMS = 108,
  // Exception parsing `QueryPreferences` from JSON
  EC_PARSE_QUERY_PREFERENCES = 109,
  // Exception parsing `SearchQuery` from JSON
  EC_PARSE_SEARCH_QUERY = 110,
  // For generic exceptions thrown by `flox::pkgdb::*` classes
  EC_PKG_DB = 111,
  // Exception converting TOML to JSON
  EC_TOML_TO_JSON = 112,
  // Exception converting YAML to JSON
  EC_YAML_TO_JSON = 113,
}; /* End enum `error_category' */


/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception
{
private:

  // Corresponds to an error_category, in this case EC_FLOX_EXCEPTION
  static constexpr std::string_view categoryMsg = "general error";
  // Additional context added when the error is thrown
  std::optional<std::string> contextMsg;
  // If some other exception was caught before throwing this one, caughtMsg
  // contains what() of that exception.
  std::optional<std::string> caughtMsg;

public:

  explicit FloxException( std::string_view contextMsg )
    : contextMsg( contextMsg )
  {}
  explicit FloxException( std::string_view contextMsg, const char * caughtMsg )
    : contextMsg( contextMsg ), caughtMsg( caughtMsg )
  {}
  [[nodiscard]] virtual error_category
  get_error_code() const noexcept
  {
    return EC_FLOX_EXCEPTION;
  };
  [[nodiscard]] virtual std::string_view
  category_message() const noexcept
  {
    return this->categoryMsg;
  };
  // We can't override what() properly because we'd need to dynamically call
  // virtual methods.
  // - We don't want to force overriding what() in every child class, because
  //   that would be a pain.
  // - We can't do that when calling what(), because what() returns a char *
  //   and is const, we don't have anywhere to store that dynamic information.
  // - We can't do it at construction time, because we can't call virtual
  //   methods.
  [[nodiscard]] std::string
  what_string() const noexcept;
};


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
