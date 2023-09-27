/* ========================================================================== *
 *
 * @file flox/core/exceptions.hh
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

enum error_category {
  EC_OKAY                  = 0
, EC_FAILURE               = 1
, EC_PKG_QUERY_INVALID_ARG = 100
};  /* End enum `error_category' */


/* -------------------------------------------------------------------------- */

/** Typed exception wrapper used for misc errors. */
class FloxException : public std::exception {
  private:
    std::string msg;
  public:
    explicit FloxException( std::string_view msg ) : msg( msg ) {}
    [[nodiscard]]
    const char * what() const noexcept override { return this->msg.c_str(); }
};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
