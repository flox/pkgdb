/* ========================================================================== *
 *
 * @file registry.cc
 *
 * @brief A set of user inputs and preferences used for resolution and search.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/flake/flakeref.hh>

#include "flox/core/util.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  std::vector<std::reference_wrapper<const std::string>>
Registry::getOrder() const
{
  std::vector<std::reference_wrapper<const std::string>> order(
    this->priority.cbegin()
  , this->priority.cend()
  );
  for ( const auto & [key, _] : this->inputs )
    {
      if ( std::find( this->priority.begin(), this->priority.end(), key )
           == this->priority.end()
         )
        {
          order.emplace_back( key );
        }
    }
  return order;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, InputPreferences & prefs )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( ( key == "subtrees" ) && ( ! value.is_null() ) )
        {
          value.get_to( prefs.subtrees );
        }
      else if ( key == "stabilities" )
        {
          value.get_to( prefs.stabilities );
        }
    }
}


  void
to_json( nlohmann::json & jto, const InputPreferences & prefs )
{
  if ( prefs.subtrees.has_value() )
    {
      jto.emplace( "subtrees", * prefs.subtrees );
    }
  else
    {
      jto.emplace( "subtrees", nullptr );
    }
  jto.emplace( "stabilities", prefs.stabilities );
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
to_json( nlohmann::json & jto, const RegistryInput & rip )
{
  to_json( jto, (InputPreferences &) rip );
  if ( rip.from == nullptr )
    {
      jto.emplace( "from", nullptr );
    }
  else
    {
      jto.emplace( "from"
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
      if ( key == "inputs" )        { value.get_to( reg.inputs );   }
      else if ( key == "defaults" ) { value.get_to( reg.defaults ); }
      else if ( key == "priority" ) { value.get_to( reg.priority ); }
    }
}


  void
to_json( nlohmann::json & jto, const Registry & reg )
{
  jto.emplace( "inputs",   reg.inputs   );
  jto.emplace( "defaults", reg.defaults );
  jto.emplace( "priority", reg.priority );
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
