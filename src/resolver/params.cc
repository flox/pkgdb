/* ========================================================================== *
 *
 * @file resolve/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 * parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, PkgDescriptorRaw & desc )
{
  // TODO
}


  void
to_json( nlohmann::json & jto, const PkgDescriptorRaw & desc )
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  void
ResolveOneParams::clear()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  bool
ResolveOneParams::fillPkgQueryArgs( const std::string         & input
                                  ,       pkgdb::PkgQueryArgs & pqa
                                  )
{
  // TODO
  return false;
}


/* -------------------------------------------------------------------------- */


  void
from_json( const nlohmann::json & jfrom, ResolveOneParams & params )
{
  // TODO
}


  void
to_json( nlohmann::json & jto, const ResolveOneParams & params )
{
  // TODO
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
