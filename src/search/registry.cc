/* ========================================================================== *
 *
 * @file registry.cc
 *
 * @brief A set of user inputs used to set input preferences during search.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/flake/flakeref.hh>

#include "flox/core/util.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, InputPreferences & prefs )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( ( key == "subtrees" ) && ( ! value.is_null() ) )
        {
          prefs.subtrees = (std::vector<subtree_type>) value;
        }
      else if ( key == "stabilities" )
        {
          prefs.stabilities = value;
        }
    }
}


  void
to_json( nlohmann::json & jfrom, const InputPreferences & prefs )
{
  if ( prefs.subtrees.has_value() )
    {
      jfrom.emplace( "subtrees", * prefs.subtrees );
    }
  else
    {
      jfrom.emplace( "subtrees", nullptr );
    }
  jfrom.emplace( "stabilities", prefs.stabilities );
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, RegistryInput & rip )
{
  from_json( jfrom, (InputPreferences &) rip );
  rip.from = std::make_shared<nix::FlakeRef>(
    jfrom.at( "from" ).get<nix::FlakeRef>()
  );
}


  void
to_json( nlohmann::json & jfrom, const RegistryInput & rip )
{
  to_json( jfrom, (InputPreferences &) rip );
  if ( rip.from == nullptr )
    {
      jfrom.emplace( "from", nullptr );
    }
  else
    {
      jfrom.emplace( "from"
                   , nix::fetchers::attrsToJSON( rip.from->toAttrs() )
                   );
    }
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, Registry & reg )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "inputs" )        { reg.inputs   = value; }
      else if ( key == "defaults" ) { reg.defaults = value; }
      else if ( key == "priority" ) { reg.priority = value; }
    }
}


  void
to_json( nlohmann::json & jfrom, const Registry & reg )
{
  jfrom.emplace( "inputs",   reg.inputs );
  jfrom.emplace( "defaults", reg.defaults );
  jfrom.emplace( "priority", reg.priority );
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
