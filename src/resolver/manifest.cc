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

/* Generate `to_json' and `from_json' `ManifestRaw::EnvBase' */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
  ManifestRaw::EnvBase
, floxhub
, dir
)


/* Generate `to_json' and `from_json' `ManifestRaw::Options' */

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
  ManifestRaw::Options::Allows
, unfree
, broken
, licenses
)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
  ManifestRaw::Options::Semver
, preferPreReleases
)

// TODO: Remap `fooBar' to `foo-bar' in the JSON.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
  ManifestRaw::Options
, systems
, allow
, semver
, packageGroupingStrategy
, activationStrategy
)


/* Generate `to_json' and `from_json' `ManifestRaw::Hook' */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
  ManifestRaw::Hook
, script
, file
)


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
          value.get_to( manifest.vars );
        }
      else if ( key == "hook" )
        {
          value.get_to( manifest.hook );
        }
      else if ( key == "options" )
        {
          value.get_to( manifest.options );
        }
      else if ( ( key == "envBase" ) || ( key == "env-base" ) )
        {
          value.get_to( manifest.envBase );
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

  void
Manifest::initPreferences()
{
  if ( this->raw.options.systems.has_value() )
    {
      this->preferences.systems = * this->raw.options.systems;
    }

  if ( this->raw.options.allow.has_value() )
    {
      if ( this->raw.options.allow->unfree.has_value() )
        {
          this->preferences.allow.unfree = * this->raw.options.allow->unfree;
        }
      if ( this->raw.options.allow->broken.has_value() )
        {
          this->preferences.allow.broken = * this->raw.options.allow->broken;
        }
      if ( this->raw.options.allow->licenses.has_value() )
        {
          this->preferences.allow.licenses =
            * this->raw.options.allow->licenses;
        }
    }

  if ( this->raw.options.semver.has_value() )
    {
      if ( this->raw.options.semver->preferPreReleases.has_value() )
        {
          this->preferences.semver.preferPreReleases =
            * this->raw.options.semver->preferPreReleases;
        }
    }
}


/* -------------------------------------------------------------------------- */

  void
Manifest::initGroups()
{
  for ( const auto & [name, desc] : this->raw.install )
    {
      if ( desc.group.has_value() )
        {
          this->groups[* desc.group].emplace( name );
        }
      else
        {
          this->groups["__ungrouped__"].emplace( name );
        }
    }
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
      this->registryRaw->inputs.emplace( "__inline__" + name
                                       , std::move( input )
                                       );
    }
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string> &
Manifest::getSystems()
{
  return this->preferences.systems;
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

  std::optional<ManifestDescriptor>
Manifest::getDescriptor( const std::string & iid )
{
  try                                { return this->raw.install.at( iid ); }
  catch( const std::out_of_range & ) { return std::nullopt; }
}


/* -------------------------------------------------------------------------- */

// TODO: this should be taking `system' as an argument so we can limit the
//       results to a maximum of 1/input.
  std::vector<Manifest::Resolved>
Manifest::resolveDescriptor( const std::string & iid )
{
  auto descriptor = this->getDescriptor( iid );
  if ( ! descriptor.has_value() )
    {
      throw FloxException( "No such package `install." + iid + "'." );
    }

  auto dbs = this->getPkgDbRegistry();

  std::vector<Manifest::Resolved> resolved;

  /* Todo list of inputs that need to be processed. */
  std::vector<std::string> inputNames;

  /* Determine which inputs we should resolve in. */
  if ( descriptor->input.has_value() )
    {
      std::visit(
        overloaded {
          [&]( const std::string & path )
            {
              (void) path;
              throw FloxException(
                "Manifest::resolveDescriptor( 'inputs." + iid + "' ): "
                "Expression files not yet supported. TODO\n"
                "  -Alex <3"
              );
            }
        , [&]( const nix::FlakeRef & ref )
            {
              if ( ref.input.getType() == "indirect" )
                {
                  inputNames.emplace_back(
                    nix::fetchers::getStrAttr( ref.input.attrs, "id" )
                  );
                }
              else
                {
                  inputNames.emplace_back( "__inline__" + iid );
                }
            }
        }
      , * descriptor->input
      );
    }
  else
    {
      for ( const auto & [name, _] : * dbs )
        {
          if ( ! hasPrefix( "__inline__", name ) )
            {
              inputNames.emplace_back( name );
            }
        }
    }

  /* Resolve that shit. */
  for ( auto & name : inputNames )
    {
      auto input = dbs->at( name );
      auto args  = this->getPkgQueryArgs( name );
      descriptor->fillPkgQueryArgs( args );
      pkgdb::PkgQuery query( args );
      auto dbRO = input->getDbReadOnly();
      auto rows = query.execute( dbRO->db );

      /* A swing and a miss. */
      if ( rows.empty() ) { continue; }

      std::string resolvedName =
        hasPrefix( "__inline__", name ) ? dbRO->getLockedFlakeRef().to_string()
                                        : name;

      for ( const auto & row : rows )
        {
          resolved.emplace_back( Resolved { .input = resolvedName
                                          , .path  = dbRO->getPackagePath( row )
                                          }
                               );
        }
    }

  return resolved;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
