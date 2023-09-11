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
#include "flox/pkgdb/input.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  std::vector<std::reference_wrapper<const std::string>>
RegistryRaw::getOrder() const
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
from_json( const nlohmann::json & jfrom, RegistryRaw & reg )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "inputs" )        { value.get_to( reg.inputs );   }
      else if ( key == "defaults" ) { value.get_to( reg.defaults ); }
      else if ( key == "priority" ) { value.get_to( reg.priority ); }
    }
}


  void
to_json( nlohmann::json & jto, const RegistryRaw & reg )
{
  jto.emplace( "inputs",   reg.inputs   );
  jto.emplace( "defaults", reg.defaults );
  jto.emplace( "priority", reg.priority );
}


/* -------------------------------------------------------------------------- */

  template <registry_input_factory FactoryType>
Registry<FactoryType>::Registry( RegistryRaw registry, FactoryType & factory )
  : registryRaw( std::move( registry ) )
{
  for ( const std::reference_wrapper<const std::string> & _name :
          this->registryRaw.getOrder()
      )
    {
      const auto & pair = std::find_if(
        this->registryRaw.inputs.begin()
      , this->registryRaw.inputs.end()
      , [&]( const auto & pair ) { return pair.first == _name.get(); }
      );

      /* Fill default/fallback values if none are defined. */
      RegistryInput input = pair->second;
      if ( ! input.subtrees.has_value() )
        {
          input.subtrees = this->registryRaw.defaults.subtrees;
        }
      if ( ! input.stabilities.has_value() )
        {
          input.stabilities = this->registryRaw.defaults.stabilities;
        }

      /* Construct the input */
      this->inputs.emplace_back(
        std::make_pair( pair->first, factory.mkInput( pair->first, input ) )
      );
    }
}


/* -------------------------------------------------------------------------- */

  template<registry_input_factory FactoryType>
  std::shared_ptr<typename FactoryType::input_type>
Registry<FactoryType>::get( const std::string & name ) const noexcept
{
  const auto maybeInput = std::find_if(
    this->inputs.begin()
  , this->inputs.end()
  , [&]( const auto & pair ) { return pair.first == name; }
  );
  if ( maybeInput == this->inputs.end() ) { return nullptr; }
  return maybeInput->second;
}


/* -------------------------------------------------------------------------- */

  template<registry_input_factory FactoryType>
  std::shared_ptr<typename FactoryType::input_type>
Registry<FactoryType>::at( const std::string & name ) const
{
  const std::shared_ptr<typename FactoryType::input_type> maybeInput =
    this->get( name );
  if ( maybeInput == nullptr )
    {
      throw std::out_of_range( "No such input '" + name + "'" );
    }
  return maybeInput;
}


/* -------------------------------------------------------------------------- */

/* Instantiate class templates for common registries. */

template class Registry<RegistryInputFactory>;
template class Registry<FloxFlakeInputFactory>;
template class Registry<pkgdb::PkgDbInputFactory>;


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
