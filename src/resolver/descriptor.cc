/* ========================================================================== *
 *
 * @file resolver/descriptor.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

ManifestDescriptor::ManifestDescriptor( const ManifestDescriptorRaw & raw )
  : name( raw.name )
  , version( raw.version )
  , optional( raw.optional )
  , group( raw.packageGroup )
{
  // You have to split `absPath` first

  // TODO: subtree
  // absPath | stability

  // TODO: systems
  // systems | absPath

  // TODO: stability
  // stability | absPath

  // TODO: path
  // path | absPath


  // TODO: input
  // packageRepository | input

}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
