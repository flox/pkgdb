/* ========================================================================== *
 *
 * @file resolver/environment.cc
 *
 * @brief A collection of files associated with an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/environment.hh"
#include "flox/core/util.hh"
#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

RegistryRaw &
Environment::getCombinedRegistryRaw()
{
  if ( ! this->combinedRegistryRaw.has_value() )
    {
      this->combinedRegistryRaw = this->globalManifest->getRegistryRaw();
      // TODO: merge registries
      // this->combinedRegistryRaw->merge( this->manifest.registryRaw );
    }
  return this->combinedRegistryRaw.value();
}


/* -------------------------------------------------------------------------- */

nix::ref<Registry<pkgdb::PkgDbInputFactory>>
Environment::getPkgDbRegistry()
{
  if ( this->dbs == nullptr )
    {
      if ( this->dbFactory == nullptr )
        {
          nix::ref<nix::Store> store = this->getStore();
          this->dbFactory = std::make_shared<pkgdb::PkgDbInputFactory>( store );
        }
      this->dbs = std::make_shared<Registry<pkgdb::PkgDbInputFactory>>(
        this->getCombinedRegistryRaw(),
        *this->dbFactory );
    }
  return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>( this->dbs );
}


/* -------------------------------------------------------------------------- */

void
Environment::fillLockedFromOldLockfile()
{
  if ( ! this->oldLockfile.has_value() ) { return; }

  for ( const auto & system : this->manifest.getSystems() )
    {
      SystemPackages sysPkgs;
      for ( const auto & [iid, desc] : this->getManifest().getDescriptors() )
        {
          /* Package is skipped for this system, so fill `std::nullopt'. */
          if ( desc.systems.has_value()
               && ( std::find( desc.systems->begin(),
                               desc.systems->end(),
                               system )
                    == desc.systems->end() ) )
            {
              sysPkgs.emplace( iid, std::nullopt );
              continue;
            }

          /* Check to see if the descriptor was modified in such a way that we
           * need to re-resolve. */
          auto oldDesc = this->oldLockfile->getDescriptors().find( iid );

          /* Skip new descriptors. */
          if ( oldDesc == this->oldLockfile->getDescriptors().end() )
            {
              continue;
            }

          /* Detect changes.
           * We ignore `group', `priority', `optional', and `systems'. */
          if ( ( desc.name != oldDesc->second.name )
               || ( desc.path != oldDesc->second.path )
               || ( desc.version != oldDesc->second.version )
               || ( desc.semver != oldDesc->second.semver )
               || ( desc.subtree != oldDesc->second.subtree )
               || ( desc.input != oldDesc->second.input ) )
            {
              continue;
            }

          // XXX: Detecting changed groups needs to be handled somewhere.

          sysPkgs.emplace(
            iid,
            this->oldLockfile->getLockfileRaw().packages.at( system ).at(
              iid ) );
        }
      this->lockfileRaw->packages.emplace( system, std::move( sysPkgs ) );
    }
}


/* -------------------------------------------------------------------------- */

std::optional<ManifestRaw>
Environment::getOldManifestRaw() const
{
  if ( this->getOldLockfile().has_value() )
    {
      return this->getOldLockfile()->getManifestRaw();
    }
  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

const Options &
Environment::getCombinedOptions()
{
  if ( ! this->combinedOptions.has_value() )
    {
      /* Start with global options ( if any ). */
      if ( this->getGlobalManifest().has_value()
           && this->getGlobalManifestRaw()->options.has_value() )
        {
          this->combinedOptions = this->getGlobalManifestRaw()->options;
        }
      else { this->combinedOptions = Options {}; }

      /* Clobber with lockfile's options ( if any ). */
      if ( this->getOldManifestRaw().has_value()
           && this->getOldManifestRaw()->options.has_value() )
        {
          this->combinedOptions->merge( *this->getOldManifestRaw()->options );
        }

      /* Clobber with manifest's options ( if any ). */
      if ( this->getManifestRaw().options.has_value() )
        {
          this->combinedOptions->merge( *this->getManifestRaw().options );
        }
    }
  return *this->combinedOptions;
}


/* -------------------------------------------------------------------------- */

const pkgdb::PkgQueryArgs &
Environment::getCombinedBaseQueryArgs()
{
  if ( ! this->combinedBaseQueryArgs.has_value() )
    {
      this->combinedBaseQueryArgs
        = static_cast<pkgdb::PkgQueryArgs>( this->getCombinedOptions() );
    }
  return *this->combinedBaseQueryArgs;
}


/* -------------------------------------------------------------------------- */

std::optional<pkgdb::row_id>
Environment::tryResolveDescriptorIn( const ManifestDescriptor & descriptor,
                                     const pkgdb::PkgDbInput &  input,
                                     const System &             system )
{
  /** Skip unrequested systems. */
  if ( descriptor.systems.has_value()
       && ( std::find( descriptor.systems->begin(),
                       descriptor.systems->end(),
                       system )
            == descriptor.systems->end() ) )
    {
      return std::nullopt;
    }

  pkgdb::PkgQueryArgs args = this->getCombinedBaseQueryArgs();
  input.fillPkgQueryArgs( args );
  descriptor.fillPkgQueryArgs( args );
  /* Limit results to the target system. */
  args.systems = std::vector<System> { system };
  pkgdb::PkgQuery query( args );
  auto            rows = query.execute( input.getDbReadOnly()->db );
  if ( rows.empty() ) { return std::nullopt; }
  return rows.front();
}


/* -------------------------------------------------------------------------- */

LockedPackageRaw
Environment::lockPackage( const LockedInputRaw & input,
                          pkgdb::PkgDbReadOnly & dbRO,
                          pkgdb::row_id          row,
                          unsigned               priority )
{
  nlohmann::json   info = dbRO.getPackage( row );
  LockedPackageRaw pkg;
  pkg.input = input;
  info.at( "absPath" ).get_to( pkg.attrPath );
  info.erase( "id" );
  info.erase( "description" );
  info.erase( "absPath" );
  info.erase( "subtree" );
  info.erase( "system" );
  info.erase( "relPath" );
  pkg.priority = priority;
  pkg.info     = std::move( info );
  return pkg;
}


/* -------------------------------------------------------------------------- */

std::optional<SystemPackages>
Environment::tryResolveGroupIn( const InstallDescriptors & group,
                                const pkgdb::PkgDbInput &  input,
                                const System &             system )
{
  std::unordered_map<InstallID, std::optional<pkgdb::row_id>> pkgRows;

  for ( const auto & [iid, descriptor] : group )
    {
      /** Skip unrequested systems. */
      if ( descriptor.systems.has_value()
           && ( std::find( descriptor.systems->begin(),
                           descriptor.systems->end(),
                           system )
                == descriptor.systems->end() ) )
        {
          pkgRows.emplace( iid, std::nullopt );
          continue;
        }

      /* Try resolving. */
      std::optional<pkgdb::row_id> maybeRow
        = this->tryResolveDescriptorIn( descriptor, input, system );
      if ( maybeRow.has_value() || descriptor.optional )
        {
          pkgRows.emplace( iid, maybeRow );
        }
      else { return std::nullopt; }
    }

  /* Convert to `LockedPackageRaw's */
  SystemPackages pkgs;
  LockedInputRaw lockedInput( input );
  auto           dbRO = input.getDbReadOnly();
  for ( const auto & [iid, maybeRow] : pkgRows )
    {
      if ( maybeRow.has_value() )
        {
          pkgs.emplace( iid,
                        Environment::lockPackage( lockedInput,
                                                  *dbRO,
                                                  *maybeRow,
                                                  group.at( iid ).priority ) );
        }
      else { pkgs.emplace( iid, std::nullopt ); }
    }

  return pkgs;
}


/* -------------------------------------------------------------------------- */

void
Environment::lockSystem( const System & system )
{
  /* This should only be called from `Environment::createLock()' after
   * initializing `lockfileRaw'. */
  assert( this->lockfileRaw.has_value() );
  SystemPackages pkgs;

  auto ungrouped = this->getManifest().getDescriptors();
  auto groups    = this->getManifest().getGroupedDescriptors();

  // TODO: Use `getCombinedRegistryRaw()'
  for ( const auto & inputPair : *this->getPkgDbRegistry() )
    {
      const auto & input = inputPair.second;
      /* Try resolving unresolved groups. */
      for ( const auto & [groupName, group] : groups )
        {
          std::optional<SystemPackages> maybeResolved
            = this->tryResolveGroupIn( group, *input, system );
          if ( maybeResolved.has_value() )
            {
              pkgs.merge( *maybeResolved );
              groups.erase( groupName );
            }
        }

      /* Try resolving unresolved ungrouped descriptors. */
      for ( const auto & [iid, descriptor] : ungrouped )
        {
          /* Skip if `systems' on descriptor excludes this system. */
          if ( descriptor.systems.has_value()
               && ( std::find( descriptor.systems->begin(),
                               descriptor.systems->end(),
                               system )
                    == descriptor.systems->end() ) )
            {
              pkgs.emplace( iid, std::nullopt );
              ungrouped.erase( iid );
              continue;
            }
          std::optional<pkgdb::row_id> maybeRow
            = this->tryResolveDescriptorIn( descriptor, *input, system );
          if ( maybeRow.has_value() )
            {
              pkgs.emplace( iid,
                            Environment::lockPackage( *input,
                                                      *maybeRow,
                                                      descriptor.priority ) );
              ungrouped.erase( iid );
            }
        }
    }

  /* Fill `std::nullopt' for skippable ungrouped descriptors. */
  for ( const auto & [iid, descriptor] : ungrouped )
    {
      if ( descriptor.optional )
        {
          pkgs.emplace( iid, std::nullopt );
          ungrouped.erase( iid );
        }
    }

  /* Throw if any ungrouped descriptors remain. */
  if ( ! ungrouped.empty() )
    {
      std::stringstream msg;
      msg << "failed to resolve some descriptor(s): ";
      bool first = true;
      for ( const auto & [iid, descriptor] : ungrouped )
        {
          if ( first ) { first = false; }
          else { msg << ", "; }
          msg << iid;
        }
      throw ResolutionFailure( msg.str() );
    }

  if ( ! groups.empty() )
    {
      std::stringstream msg;
      msg << "failed to resolve some groups(s):";
      for ( const auto & [groupName, group] : groups )
        {
          bool first = true;
          msg << std::endl << "  " << groupName << ": ";
          for ( const auto & [iid, descriptor] : group )
            {
              if ( first ) { first = false; }
              else { msg << ", "; }
              msg << iid;
            }
        }
      throw ResolutionFailure( msg.str() );
    }

  this->lockfileRaw->packages.emplace( system, std::move( pkgs ) );
}


/* -------------------------------------------------------------------------- */

Lockfile
Environment::createLockfile()
{
  if ( ! this->lockfileRaw.has_value() )
    {
      this->lockfileRaw = LockfileRaw {};
      // TODO: Make a better implementation of `fillLockedFromOldLockfile()',
      //       and modify `lockSystem()' to only target unlocked descriptors.
      //this->fillLockedFromOldLockfile();
      this->lockfileRaw->manifest = this->getManifestRaw();
      // TODO: Once you figure out `getCombinedRegistryRaw' you might need to
      //       remove some unused registry members.
      this->lockfileRaw->registry
        = this->getManifest().getLockedRegistry( this->getStore() );
      for ( const auto & system : this->getSystems() )
        {
          this->lockSystem( system );
        }
    }
  return Lockfile( *this->lockfileRaw );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
