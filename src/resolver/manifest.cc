/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "install" )
        {
          for ( const auto & [name, desc] : value.items() )
            {
              /* An empty/null descriptor uses `name' of the attribute. */
              if ( desc.is_null() )
                {
                  ManifestDescriptor manDesc;
                  manDesc.name = name;
                  manifest.install.emplace( name, std::move( manDesc ) );
                }
              else
                {
                  manifest.install.emplace(
                    name
                  , ManifestDescriptor( desc.get<ManifestDescriptorRaw>() )
                  );
                }
            }
        }
      else if ( key == "registry" )
        {
          value.get_to( manifest.registry );
        }
      else if ( key == "vars" )
        {
          // TODO
          continue;
        }
      else if ( key == "hook" )
        {
          // TODO
          continue;
        }
      else if ( key == "options" )
        {
          ManifestRaw::Options options;
          for ( const auto & [okey, ovalue] : value.items() )
            {
              if ( okey == "systems" )
                {
                  ovalue.get_to( options.systems );
                }
              else
                {
                  throw FloxException(
                    "Unrecognized manifest field: `options." + key + "'."
                  );
                }
              // TODO: others
            }
          manifest.options = std::move( options );
        }
      else if ( key == "envBase" )
        {
          // TODO
          continue;
        }
      else
        {
          throw FloxException( "Unrecognized manifest field: `" + key + "'." );
        }
    }
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<pkgdb::PkgDbInput>
ManifestInputFactory::mkInput( const std::string   & name
                             , const RegistryInput & input
                             )
{
  if ( hasPrefix( "__inline__", name ) )
    {
      return std::make_shared<pkgdb::PkgDbInput>(
        this->store
      , input
      , this->cacheDir
      );
    }
  else
    {
      return std::make_shared<pkgdb::PkgDbInput>(
        this->store
      , input
      , this->cacheDir
      , name
      );
    }
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Get a _base_ set of query arguments for the input associated with
 *        @a name and declared @a preferences.
 */
  [[nodiscard]]
  pkgdb::PkgQueryArgs
Manifest::getPkgQueryArgs( const std::string & name )
{
  pkgdb::PkgQueryArgs args;
  args.systems = this->getSystems();
  this->initRegistry();
  this->registry->at( name )->fillPkgQueryArgs( args );
  return args;
}


/* -------------------------------------------------------------------------- */

  std::unordered_map<std::string, nix::FlakeRef>
Manifest::getInlineInputs() const
{
  std::unordered_map<std::string, nix::FlakeRef> inputs;

  for ( const auto & [name, desc] : this->raw.install )
    {
      if ( desc.input.has_value() &&
           std::holds_alternative<nix::FlakeRef>( * desc.input )
         )
        {
          auto ref = std::get<nix::FlakeRef>( * desc.input );
          if ( ref.input.getType() != "indirect" )
            {
              inputs.emplace( name, std::move( ref ) );
            }
        }
    }

  return inputs;
}


/* -------------------------------------------------------------------------- */

  // TODO: Merge this with `getInlineInputs' probably.
  void
Manifest::initRegistryRaw()
{
  if ( this->registryRaw.has_value() ) { return; }
  this->registryRaw = this->raw.registry;
  for ( const auto & [name, ref] : this->getInlineInputs() )
    {
      RegistryInput input;
      input.from = std::make_shared<nix::FlakeRef>( ref );
      this->registryRaw->inputs.emplace( "__indirect__" + name
                                       , std::move( input )
                                       );
    }
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string> &
Manifest::getSystems()
{
  /* Lazily fill */
  if ( this->systems.empty() )
    {
      if ( this->raw.options.has_value() &&
           this->raw.options->systems.has_value()
         )
        {
          this->systems = * this->raw.options->systems;
        }
      else
        {
          /* Fallback to current system. */
          this->systems = std::vector<std::string> {
            nix::settings.thisSystem.get()
          };
        }
    }
  return this->systems;
}


/* -------------------------------------------------------------------------- */

  RegistryRaw
Manifest::getRegistryRaw()
{
  this->initRegistryRaw();
  return * this->registryRaw;
}


/* -------------------------------------------------------------------------- */

  std::unordered_map<std::string, nix::FlakeRef>
Manifest::getLockedInputs()
{
  std::unordered_map<std::string, nix::FlakeRef> inputs;
  for ( const auto & [name, input] : * this->getPkgDbRegistry() )
    {
      inputs.emplace( name
                    , input->getFlake()->lockedFlake.flake.lockedRef
                    );
    }
  return inputs;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
