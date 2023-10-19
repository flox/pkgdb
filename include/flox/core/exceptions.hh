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
  EC_FAILURE               = 1,
  EC_FLOX_EXCEPTION        = 100,
  EC_PKG_QUERY_INVALID_ARG = 101,
  EC_TOML_TO_JSON          = 102
}; /* End enum `error_category' */


/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception
{
private:

  // Corresponds to an error_category, in this case EC_FLOX_EXCEPTION
  static constexpr std::string_view categoryMsg
    = "error encountered running pkgdb";
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
  error_code() const noexcept
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
  std::string
  what_string() const noexcept;
};


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
