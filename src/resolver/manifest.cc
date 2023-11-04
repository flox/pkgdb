/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <optional>

#include "compat/concepts.hh"
#include "flox/core/util.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Read a flox::resolver::GlobalManifest or flox::resolver::Manifest
 *        from a file.
 */
template<typename ManifestType>
static ManifestType
readManifestFromPath( const std::filesystem::path &manifestPath )
  requires std::is_base_of<GlobalManifestRaw, ManifestType>::value
{
  if ( ! std::filesystem::exists( manifestPath ) )
    {
      throw InvalidManifestFileException( "no such path: "
                                          + manifestPath.string() );
    }
  return readAndCoerceJSON( manifestPath );
}


/* -------------------------------------------------------------------------- */

void
GlobalManifest::init()
{
  this->manifestRaw.check();
  if ( this->manifestRaw.registry.has_value() )
    {
      this->registryRaw = *this->manifestRaw.registry;
    }
}


/* -------------------------------------------------------------------------- */

GlobalManifest::GlobalManifest( std::filesystem::path manifestPath )
  : manifestPath( std::move( manifestPath ) )
  , manifestRaw( readManifestFromPath<GlobalManifestRaw>( this->manifestPath ) )
{
  this->init();
}


/* -------------------------------------------------------------------------- */

void
Manifest::init()
{
  this->manifestRaw.check();
  if ( this->manifestRaw.registry.has_value() )
    {
      this->registryRaw = *this->manifestRaw.registry;
    }

  if ( ! this->manifestRaw.install.has_value() ) { return; }
  for ( const auto &[iid, raw] : *this->manifestRaw.install )
    {
      /* An empty/null descriptor uses `name' of the attribute. */
      if ( raw.has_value() )
        {
          this->descriptors.emplace( iid, ManifestDescriptor( iid, *raw ) );
        }
      else
        {
          ManifestDescriptor manDesc;
          manDesc.name = iid;
          this->descriptors.emplace( iid, std::move( manDesc ) );
        }
    }
}


/* -------------------------------------------------------------------------- */

Manifest::Manifest( std::filesystem::path manifestPath, ManifestRaw raw )
{
  this->manifestPath = std::move( manifestPath );
  this->manifestRaw  = std::move( raw );
  this->init();
}


/* -------------------------------------------------------------------------- */

Manifest::Manifest( std::filesystem::path manifestPath )
{
  this->manifestPath = std::move( manifestPath );
  this->manifestRaw  = readManifestFromPath<ManifestRaw>( this->manifestPath );
  this->init();
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs
GlobalManifest::getBaseQueryArgs() const
{
  pkgdb::PkgQueryArgs args;
  if ( ! this->manifestRaw.options.has_value() ) { return args; }

  if ( this->manifestRaw.options->systems.has_value() )
    {
      args.systems = *this->manifestRaw.options->systems;
    }

  if ( this->manifestRaw.options->allow.has_value() )
    {
      if ( this->manifestRaw.options->allow->unfree.has_value() )
        {
          args.allowUnfree = *this->manifestRaw.options->allow->unfree;
        }
      if ( this->manifestRaw.options->allow->broken.has_value() )
        {
          args.allowBroken = *this->manifestRaw.options->allow->broken;
        }
      args.licenses = this->manifestRaw.options->allow->licenses;
    }

  if ( this->manifestRaw.options->semver.has_value()
       && this->manifestRaw.options->semver->preferPreReleases.has_value() )
    {
      args.preferPreReleases
        = *this->manifestRaw.options->semver->preferPreReleases;
    }
  return args;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
ManifestFileMixin::addManifestFileOption( argparse::ArgumentParser &parser )
{
  return parser.add_argument( "--manifest" )
    .help( "The path to the 'manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string &strPath )
             { this->manifestPath = nix::absPath( strPath ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
ManifestFileMixin::addManifestFileArg( argparse::ArgumentParser &parser,
                                       bool                      required )
{
  argparse::Argument &arg
    = parser.add_argument( "manifest" )
        .help( "The path to a 'manifest.{toml,yaml,json}' file." )
        .metavar( "MANIFEST-PATH" )
        .action( [&]( const std::string &strPath )
                 { this->manifestPath = nix::absPath( strPath ); } );
  return required ? arg.required() : arg;
}


/* -------------------------------------------------------------------------- */

std::filesystem::path
ManifestFileMixin::getManifestPath()
{
  if ( ! this->manifestPath.has_value() )
    {
      throw InvalidManifestFileException( "no manifest path given" );
    }
  return *this->manifestPath;
}


/* -------------------------------------------------------------------------- */

const Manifest &
ManifestFileMixin::getManifest()
{
  if ( ! this->manifest.has_value() )
    {
      this->manifest = Manifest( this->getManifestPath() );
    }
  return *this->manifest;
}


/* -------------------------------------------------------------------------- */

const RegistryRaw &
ManifestFileMixin::getLockedRegistry()
{
  if ( ! this->lockedRegistry.has_value() )
    {
      this->lockedRegistry
        = this->getManifest().getLockedRegistry( this->getStore() );
    }
  return *this->lockedRegistry;
}


/* -------------------------------------------------------------------------- */

const pkgdb::PkgQueryArgs &
ManifestFileMixin::getBaseQueryArgs()
{
  if ( ! this->baseQueryArgs.has_value() )
    {
      this->baseQueryArgs = this->getManifest().getBaseQueryArgs();
    }
  return *this->baseQueryArgs;
}


/* -------------------------------------------------------------------------- */

static Resolved
mkResolved( std::string_view         inputName,
            const pkgdb::PkgDbInput &input,
            pkgdb::row_id            row,
            unsigned                 priority )
{
  auto info = input.getDbReadOnly()->getPackage( row );
  return { .input = Resolved::Input( std::string( inputName ),
                                     input.getFlakeRef()->to_string() ),
           .path  = std::move( info.at( "absPath" ) ),
           .info  = { { "pname", std::move( info.at( "pname" ) ) },
                      { "version", std::move( info.at( "version" ) ) },
                      { "license", std::move( info.at( "license" ) ) },
                      { "priority", priority } } };
}


/* -------------------------------------------------------------------------- */

const std::unordered_map<std::string, std::optional<Resolved>> &
ManifestFileMixin::lockUngroupedDescriptor( const std::string        &iid,
                                            const ManifestDescriptor &desc )
{
  /* This interface is only intended for ungrouped descriptors. */
  assert( ! desc.group.has_value() );

  /* See if it was already locked. */
  if ( auto locked = this->lockedDescriptors.find( iid );
       locked != this->lockedDescriptors.end() )
    {
      return locked->second;
    }

  /* For each system we need to set identify an input `absPath` to
   * resolve a descriptor to _real_ package/derivation. */
  std::unordered_map<std::string, std::optional<Resolved>> locked;

  pkgdb::PkgQueryArgs baseArgs = this->getBaseQueryArgs();

  /* Iterate over the systems. */
  for ( const auto &system : this->getSystems() )
    {
      /* See if this system is skippable. */
      if ( desc.systems.has_value()
           && ( std::find( desc.systems->begin(), desc.systems->end(), system )
                == desc.systems->end() ) )
        {
          /* This descriptor is not for this system. */
          locked.emplace( system, std::nullopt );
          continue;
        }

      /* Otherwise, try to lock it with an input. */
      if ( desc.input.has_value() )
        {
          // TODO: Post-GA this is allowed!
          throw InvalidManifestFileException(
            "You cannot specify an input in manifest files." );
        }

      /* Prep our query. */
      pkgdb::PkgQueryArgs args = baseArgs;
      desc.fillPkgQueryArgs( args );
      args.systems = { system };
      pkgdb::PkgQuery query( args );

      // TODO: handle groups.

      /* Try each input. */
      bool found = false;
      for ( const auto &[name, input] : *this->getPkgDbRegistry() )
        {
          auto rows = query.execute( input->getDbReadOnly()->db );
          if ( rows.empty() ) { continue; }

          found = true;
          locked.emplace(
            system,
            mkResolved( name, *input, rows.front(), desc.priority ) );
          break;
        }

      if ( ! found )
        {
          /* No match. */
          if ( desc.optional )
            {
              locked.emplace( system, std::nullopt );
              continue;
            }
          // TODO: make this a typed exception.
          throw FloxException( "Failed to resolve descriptor for `install."
                               + iid + "' on system `" + system + "'." );
        }
    }

  /* Stash it in the collection. */
  this->lockedDescriptors.emplace( iid, std::move( locked ) );

  return this->lockedDescriptors.at( iid );
}


/* -------------------------------------------------------------------------- */

void
ManifestFileMixin::checkGroups()
{
  std::unordered_map<GroupName,
                     std::unordered_map<System, std::optional<Resolved::Input>>>
    groupInputs;
  for ( const auto &[iid, systemsResolved] : this->lockedDescriptors )
    {
      auto maybeGroupName = this->getDescriptors().at( iid ).group;
      /* Skip if the descriptor doesn't name a group. */
      if ( ! maybeGroupName.has_value() ) { continue; }
      /* Either define the group or verify alignment. */
      auto maybeGroup = groupInputs.find( *maybeGroupName );
      if ( maybeGroup == groupInputs.end() )
        {
          std::unordered_map<std::string, std::optional<Resolved::Input>>
            inputs;
          for ( const auto &[system, resolved] : systemsResolved )
            {
              if ( resolved.has_value() )
                {
                  inputs.emplace( system, resolved->input );
                }
              else { inputs.emplace( system, std::nullopt ); }
            }
          groupInputs.emplace( maybeGroup->first, std::move( inputs ) );
        }
      else
        {
          for ( const auto &[system, resolved] : systemsResolved )
            {
              if ( resolved.has_value() )
                {
                  /* If the previous declaration skipped a system, we
                   * may fill it.
                   * Otherwise assert equality. */
                  if ( ! maybeGroup->second.at( system ).has_value() )
                    {
                      maybeGroup->second.at( system ) = resolved->input;
                    }
                  else if ( ( *maybeGroup->second.at( system ) ).locked
                            != resolved->input.locked )
                    {
                      // TODO: make a new exception
                      throw FloxException(
                        "locked descriptor `packages." + iid + "." + system
                        + "' does not align with other members of its group" );
                    }
                }
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

const std::unordered_map<
  std::string,
  std::unordered_map<std::string, std::optional<Resolved>>> &
ManifestFileMixin::getLockedDescriptors()
{
  /* First we handle ungrouped descriptors, and collect those which are members
   * of a group to be handled later. */
  std::unordered_map<std::string,                    /* group name */
                     std::unordered_set<std::string> /* _install ID_ */
                     >
    groupsTodo;
  for ( const auto &[iid, desc] : this->getDescriptors() )
    {
      if ( desc.group.has_value() )
        {
          groupsTodo.try_emplace( *desc.group,
                                  std::unordered_set<std::string> {} );
          groupsTodo.at( *desc.group ).emplace( iid );
        }
      else
        {
          lockedDescriptors.emplace(
            iid,
            this->lockUngroupedDescriptor( iid, desc ) );
        }
    }

  // TODO: When you start allowing `packageRepository' you need to assert that
  //       no two group members declare different repos.

  for ( const auto &[group, iids] : groupsTodo )
    {
      /* First check to see if any member of the group already has a
       * `lockedDescriptor' entry ( potentially filled by a lockfile ).
       * If they do, align unlocked group members to the same rev. */
      std::unordered_map<System, std::optional<Resolved::Input>> input;
      bool hadInput = false;
      for ( const auto &iid : iids )
        {
          auto maybeLocked = this->lockedDescriptors.find( iid );
          if ( maybeLocked != this->lockedDescriptors.end() )
            {
              hadInput = true;
              for ( const auto &[system, resolved] : maybeLocked->second )
                {
                  input.try_emplace( system, std::nullopt );
                  if ( resolved.has_value() )
                    {
                      input.at( system )
                        = input.at( system ).value_or( resolved->input );
                    }
                }
            }
        }

      /* If you had a locked peer, try resolving everyone else to that input. */
      if ( hadInput )
        {
          pkgdb::PkgQueryArgs baseArgs = this->getBaseQueryArgs();
          for ( const auto &iid : iids )
            {
              /* Skip if the descriptor is already locked. */
              auto maybeLocked = this->lockedDescriptors.find( iid );
              if ( maybeLocked != this->lockedDescriptors.end() ) { continue; }

              std::unordered_map<std::string, std::optional<Resolved>> locked;

              pkgdb::PkgQueryArgs args = baseArgs;
              auto                desc = this->getDescriptors().at( iid );

              for ( const auto &system : this->getSystems() )
                {
                  /* See if this system is skippable. */
                  if ( desc.systems.has_value()
                       && ( std::find( desc.systems->begin(),
                                       desc.systems->end(),
                                       system )
                            == desc.systems->end() ) )
                    {
                      /* This descriptor is not for this system. */
                      locked.emplace( system, std::nullopt );
                      continue;
                    }

                  /* Otherwise, try to lock it with an input. */
                  if ( desc.input.has_value() )
                    {
                      // TODO: Post-GA this is allowed!
                      throw InvalidManifestFileException(
                        "You cannot specify an input in manifest files." );
                    }

                  /* Prep our query. */
                  pkgdb::PkgQueryArgs args = baseArgs;
                  desc.fillPkgQueryArgs( args );
                  args.systems = { system };
                  pkgdb::PkgQuery query( args );

                  auto registryInput = this->getPkgDbRegistry()->at(
                    input.at( system )->name.value() );

                  auto rows
                    = query.execute( registryInput->getDbReadOnly()->db );
                }
            }
        }
    }

  this->checkGroups();
  return this->lockedDescriptors;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
