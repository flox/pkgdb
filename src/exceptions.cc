/* ========================================================================== *
 *
 * @file flox/exceptions.cc
 *
 * @brief Definitions of various `std::exception` children used for throwing
 *        errors with nice messages and typed discrimination.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "flox/core/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */
std::string
FloxException::what_string() const noexcept
{
  std::string msg( this->getCategoryMessage() );
  if ( this->contextMsg.has_value() )
    {
      msg += ": ";
      msg += *( this->contextMsg );
    }
  if ( this->caughtMsg.has_value() )
    {
      msg += ": ";
      msg += *( this->caughtMsg );
    }
  return msg;
}

/* -------------------------------------------------------------------------- */

void
to_json( nlohmann::json & jto, const FloxException & err )
{
  std::string msg;
  if ( err.contextMsg.has_value() )
    {
      msg += ": ";
      msg += *( err.contextMsg );
    }
  if ( err.caughtMsg.has_value() )
    {
      msg += ": ";
      msg += *( err.caughtMsg );
    }

  jto = {
    { "exit_code", err.get_error_code() },
    { "message", msg },
    { "category", err.category_message() },
  };
}

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
