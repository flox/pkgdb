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

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
