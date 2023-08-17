/* ========================================================================== *
 *
 * @file flox/exceptions.hh
 *
 * @brief Definitions of various `std::exception` children used for throwing
 *        throwing errors with nice messages and typed discrimination.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <stdexcept>
#include <string>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception {
  private:
    std::string msg;
  public:
    FloxException( std::string_view msg ) : msg( msg ) {}
    const char * what() const noexcept { return this->msg.c_str(); }
};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
