/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <stdexcept>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class ResolverException : public std::exception {
  private:
    std::string msg;
  public:
    ResolverException( std::string_view msg ) : msg( msg ) {}
    const char * what() const noexcept { return this->msg.c_str(); }
};


/* -------------------------------------------------------------------------- */

class DescriptorException : public ResolverException {
  public:
    DescriptorException( std::string_view msg )
      : ResolverException( msg )
    {}
};


/* -------------------------------------------------------------------------- */

class CacheException : public ResolverException {
  public:
    CacheException( std::string_view msg )
      : ResolverException( msg )
    {}
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
