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

void
to_json( nlohmann::json & jto, const FloxException & err )
{
  auto        contextMsg = err.getContextMessage();
  auto        caughtMsg  = err.getCaughtMessage();
  std::string msg;
  if ( contextMsg.has_value() )
    {
      msg = *contextMsg;
      if ( caughtMsg.has_value() )
        {
          msg += ": ";
          msg += *caughtMsg;
        }
    }
  else if ( caughtMsg.has_value() ) { msg = *caughtMsg; }

  jto = {
    { "exit_code", err.getErrorCode() },
    { "message", std::move( msg ) },
    { "category", err.getCategoryMessage() },
  };
}

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
