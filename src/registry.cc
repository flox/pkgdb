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
#include "flox/pkgdb/input.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

void
InputPreferences::clear()
{
  this->subtrees    = std::nullopt;
  this->stabilities = std::nullopt;
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
InputPreferences::fillPkgQueryArgs( pkgdb::PkgQueryArgs &pqa ) const
{
  pqa.subtrees    = this->subtrees;
  pqa.stabilities = this->stabilities;
  return pqa;
}


/* -------------------------------------------------------------------------- */

void
RegistryRaw::clear()
{
  this->inputs.clear();
  this->priority.clear();
  this->defaults.clear();
}


/* -------------------------------------------------------------------------- */

std::vector<std::reference_wrapper<const std::string>>
RegistryRaw::getOrder() const
{
  std::vector<std::reference_wrapper<const std::string>> order(
    this->priority.cbegin(),
    this->priority.cend() );
  for ( const auto &[key, _] : this->inputs )
    {
      if ( std::find( this->priority.begin(), this->priority.end(), key )
           == this->priority.end() )
        {
          order.emplace_back( key );
        }
    }
  return order;
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, RegistryInput &rip )
{
  from_json( jfrom, dynamic_cast<InputPreferences &>( rip ) );
  rip.from = std::make_shared<nix::FlakeRef>(
    jfrom.at( "from" ).get<nix::FlakeRef>() );
}


void
to_json( nlohmann::json &jto, const RegistryInput &rip )
{
  to_json( jto, dynamic_cast<const InputPreferences &>( rip ) );
  if ( rip.from == nullptr ) { jto.emplace( "from", nullptr ); }
  else
    {
      jto.emplace( "from", nix::fetchers::attrsToJSON( rip.from->toAttrs() ) );
    }
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
RegistryRaw::fillPkgQueryArgs( const std::string   &input,
                               pkgdb::PkgQueryArgs &pqa ) const
{
  /* Look for the named input and our fallbacks/default in the inputs list.
   * then fill input specific settings. */
  try
    {
      const RegistryInput &minput = this->inputs.at( input );
      pqa.subtrees    = minput.subtrees.has_value() ? minput.subtrees
                                                    : this->defaults.subtrees;
      pqa.stabilities = minput.stabilities.has_value()
                          ? minput.stabilities
                          : this->defaults.stabilities;
    }
  catch ( ... )
    {
      pqa.subtrees    = this->defaults.subtrees;
      pqa.stabilities = this->defaults.stabilities;
    }
  return pqa;
}


/* -------------------------------------------------------------------------- */

nix::ref<FloxFlake>
FloxFlakeInput::getFlake()
{
  if ( this->flake == nullptr )
    {
      this->flake
        = std::make_shared<FloxFlake>( NixState( this->store ).getState(),
                                       *this->getFlakeRef() );
    }
  return static_cast<nix::ref<FloxFlake>>( this->flake );
}


/* -------------------------------------------------------------------------- */

const std::vector<Subtree> &
FloxFlakeInput::getSubtrees()
{
  if ( ! this->enabledSubtrees.has_value() )
    {
      if ( this->subtrees.has_value() )
        {
          this->enabledSubtrees = *this->subtrees;
        }
      else
        {
          try
            {
              auto root = this->getFlake()->openEvalCache()->getRoot();
              if ( root->maybeGetAttr( "catalog" ) != nullptr )
                {
                  this->enabledSubtrees = std::vector<Subtree> { ST_CATALOG };
                }
              else if ( root->maybeGetAttr( "packages" ) != nullptr )
                {
                  this->enabledSubtrees = std::vector<Subtree> { ST_PACKAGES };
                }
              else if ( root->maybeGetAttr( "legacyPackages" ) != nullptr )
                {
                  this->enabledSubtrees = std::vector<Subtree> { ST_LEGACY };
                }
              else { this->enabledSubtrees = std::vector<Subtree> {}; }
            }
          catch ( const nix::EvalError &err )
            {
              throw NixEvalException( "could not determine flake subtrees",
                                      err );
            }
        }
    }
  return *this->enabledSubtrees;
}


/* -------------------------------------------------------------------------- */

RegistryInput
FloxFlakeInput::getLockedInput()
{
  return { this->getSubtrees(),
           this->stabilities,
           this->getFlake()->lockedFlake.flake.lockedRef };
}


/* -------------------------------------------------------------------------- */

std::map<std::string, RegistryInput>
FlakeRegistry::getLockedInputs()
{
  std::map<std::string, RegistryInput> locked;
  for ( auto &[name, input] : *this )
    {
      locked.emplace( name, input->getLockedInput() );
    }
  return locked;
}


/* -------------------------------------------------------------------------- */

RegistryRaw
lockRegistry( const RegistryRaw &unlocked, nix::ref<nix::Store> store )
{
  auto factory  = FloxFlakeInputFactory( store );
  auto locked   = unlocked;
  locked.inputs = FlakeRegistry( unlocked, factory ).getLockedInputs();
  return locked;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
