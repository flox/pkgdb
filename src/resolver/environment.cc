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
      if ( this->globalManifest.has_value() )
        {
          this->combinedRegistryRaw = this->globalManifest->getRegistryRaw();
          this->combinedRegistryRaw.value().merge(
            this->manifest.getLockedRegistry() );
        }
      else { this->combinedRegistryRaw = this->manifest.getLockedRegistry(); }
    }
  return *this->combinedRegistryRaw;
}


/* -------------------------------------------------------------------------- */

nix::ref<Registry<pkgdb::PkgDbInputFactory>>
Environment::getPkgDbRegistry()
{
  if ( this->dbs == nullptr )
    {
      nix::ref<nix::Store> store   = this->getStore();
      auto                 factory = pkgdb::PkgDbInputFactory( store );
      this->dbs = std::make_shared<Registry<pkgdb::PkgDbInputFactory>>(
        this->getCombinedRegistryRaw(),
        factory );
      /* Scrape if needed. */
      for ( auto & [name, input] : *this->dbs )
        {
          input->scrapeSystems( this->getSystems() );
        }
    }
  return static_cast<nix::ref<Registry<pkgdb::PkgDbInputFactory>>>( this->dbs );
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

// Helper function for groupIsLocked.
// A system is skipped if systems is specified but that system is not.
bool
systemSkipped( const System &                             system,
               const std::optional<std::vector<System>> & systems )
{
  return systems.has_value()
         && ( std::find( systems->begin(), systems->end(), system )
              == systems->end() );
}

/* -------------------------------------------------------------------------- */

bool
Environment::groupIsLocked( const InstallDescriptors & group,
                            const Lockfile &           oldLockfile,
                            const System &             system )
{
  InstallDescriptors oldDescriptors = oldLockfile.getDescriptors();
  SystemPackages     oldSystemPackages
    = oldLockfile.getLockfileRaw().packages.at( system );

  for ( auto & [iid, descriptor] : group )
    {
      // If the descriptor has changed compared to the one in the lockfile
      // manifest, it needs to be locked again.
      if ( auto oldDescriptorPair = oldDescriptors.find( iid );
           oldDescriptorPair != oldDescriptors.end() )
        {
          auto & [_, oldDescriptor] = *oldDescriptorPair;
          /* We ignore `priority' and handle `systems' below. */
          if ( ( descriptor.name != oldDescriptor.name )
               || ( descriptor.path != oldDescriptor.path )
               || ( descriptor.version != oldDescriptor.version )
               || ( descriptor.semver != oldDescriptor.semver )
               || ( descriptor.subtree != oldDescriptor.subtree )
               || ( descriptor.input != oldDescriptor.input )
               || ( descriptor.group != oldDescriptor.group )
               || ( descriptor.optional != oldDescriptor.optional ) )
            {
              return false;
            }
          // Ignore changes to systems other than the one we're locking
          if ( systemSkipped( system, descriptor.systems )
               != systemSkipped( system, oldDescriptor.systems ) )
            {
              return false;
            }
        }
      /* If the descriptor doesn't even exist in the lockfile manifest, it needs
       * to be locked again. */
      else { return false; }
      // Check if the descriptor exists in the lockfile lock
      if ( auto oldLockedPackagePair = oldSystemPackages.find( iid );
           oldLockedPackagePair != oldSystemPackages.end() )
        {
          // Note: we could relock if the prior locking attempt was null
          // auto & [_, oldLockedPackage] = *oldLockedPackagePair;
          // if ( !oldLockedPackage.has_value()) { return false; }
        }
      // If the descriptor doesn't even exist in the lockfile lock, it needs to
      // be locked again.
      else { return false; }
    }
  // We haven't found something unlocked, so everything must be locked.
  return true;
}

/* -------------------------------------------------------------------------- */

std::vector<InstallDescriptors>
Environment::getUnlockedGroups( const System & system )
{
  auto lockfile           = this->getOldLockfile();
  auto groupedDescriptors = this->getManifest().getGroupedDescriptors();
  if ( ! lockfile.has_value() ) { return groupedDescriptors; }

  for ( auto group = groupedDescriptors.begin();
        group != groupedDescriptors.end(); )
    {
      if ( groupIsLocked( *group, *lockfile, system ) )
        {
          group = groupedDescriptors.erase( group );
        }
      else { ++group; }
    }

  return groupedDescriptors;
}

/* -------------------------------------------------------------------------- */

std::vector<InstallDescriptors>
Environment::getLockedGroups( const System & system )
{
  auto lockfile = this->getOldLockfile();
  if ( ! lockfile.has_value() ) { return std::vector<InstallDescriptors> {}; }

  auto groupedDescriptors = this->getManifest().getGroupedDescriptors();

  // remove all groups that are ~not~ already locked
  for ( auto group = groupedDescriptors.begin();
        group != groupedDescriptors.end(); )
    {
      if ( ! groupIsLocked( *group, *lockfile, system ) )
        {
          group = groupedDescriptors.erase( group );
        }
      else { ++group; }
    }

  return groupedDescriptors;
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

  auto groups = this->getUnlockedGroups( system );

  // TODO: Use `getCombinedRegistryRaw()'
  for ( const auto & [_, input] : *this->getPkgDbRegistry() )
    {
      /* Try resolving unresolved groups. */
      for ( auto group = groups.begin(); group != groups.end(); )
        {
          std::optional<SystemPackages> maybeResolved
            = this->tryResolveGroupIn( *group, *input, system );
          if ( maybeResolved.has_value() )
            {
              pkgs.merge( *maybeResolved );
              group = groups.erase( group );
            }
          else { ++group; }
        }
    }

  if ( ! groups.empty() )
    {
      std::stringstream msg;
      msg << "failed to resolve some packages(s):";
      for ( const auto & group : groups )
        {
          // TODO tryResolveGroupIn should report which packages failed to
          // resolve
          if ( auto descriptor = group.begin();
               descriptor != group.end()
               && descriptor->second.group.has_value() )
            {

              msg << std::endl
                  << "  some package in group " << *descriptor->second.group
                  << "failed to resolve: ";
            }
          else
            {
              msg << std::endl << "  one of the following failed to resolve: ";
            }
          bool first = true;
          for ( const auto & [iid, descriptor] : group )
            {
              if ( first ) { first = false; }
              else { msg << ", "; }
              msg << iid;
            }
        }
      throw ResolutionFailure( msg.str() );
    }


  // Copy over old lockfile entries we want to keep
  if ( auto oldLockfile = this->getOldLockfile() )
    {
      SystemPackages systemPackages
        = oldLockfile->getLockfileRaw().packages.at( system );
      auto oldDescriptors = oldLockfile->getDescriptors();
      for ( const auto & group : this->getLockedGroups( system ) )
        {
          for ( const auto & [iid, descriptor] : group )
            {
              if ( auto oldLockedPackagePair = systemPackages.find( iid );
                   oldLockedPackagePair != systemPackages.end() )
                {
                  pkgs.emplace( *oldLockedPackagePair );
                }
            }
        }
    }

  this->lockfileRaw->packages.emplace( system, std::move( pkgs ) );
}


/* -------------------------------------------------------------------------- */

Lockfile
Environment::createLockfile()
{
  if ( ! this->lockfileRaw.has_value() )
    {
      this->lockfileRaw           = LockfileRaw {};
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

const std::optional<GlobalManifest> &
EnvironmentMixin::getGlobalManifest()
{
  if ( ( ! this->globalManifest.has_value() )
       && this->globalManifestPath.has_value() )
    {
      this->globalManifest = GlobalManifest( *this->globalManifestPath );
    }
  return this->globalManifest;
}


/* -------------------------------------------------------------------------- */

const Manifest &
EnvironmentMixin::getManifest()
{
  if ( ! this->manifest.has_value() )
    {
      if ( ! this->manifestPath.has_value() )
        {
          throw InvalidManifestFileException(
            "you must provide the path to a manifest file" );
        }
      this->manifest = Manifest( *this->manifestPath );
    }
  return *this->manifest;
}


/* -------------------------------------------------------------------------- */

const std::optional<Lockfile> &
EnvironmentMixin::getLockfile()
{
  if ( ( ! this->lockfile.has_value() ) && this->lockfilePath.has_value() )
    {
      this->lockfile = Lockfile( *this->lockfilePath );
    }
  return this->lockfile;
}


/* -------------------------------------------------------------------------- */

Environment &
EnvironmentMixin::getEnvironment()
{
  if ( ! this->environment.has_value() )
    {
      this->environment = Environment( this->getGlobalManifest(),
                                       this->getManifest(),
                                       this->getLockfile() );
    }
  return *this->environment;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addGlobalManifestFileOption(
  argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--global-manifest" )
    .help( "The path to the user's global `manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initGlobalManifestPath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addManifestFileOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--manifest" )
    .help( "The path to the `manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initManifestPath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addManifestFileArg( argparse::ArgumentParser & parser,
                                      bool                       required )
{
  argparse::Argument & arg
    = parser.add_argument( "manifest" )
        .help( "The path to the project's `manifest.{toml,yaml,json}' file." )
        .metavar( "MANIFEST-PATH" )
        .action( [&]( const std::string & strPath )
                 { this->initManifestPath( nix::absPath( strPath ) ); } );
  return required ? arg.required() : arg;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
EnvironmentMixin::addLockfileOption( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--lockfile" )
    .help( "The path to the projects existing `manifest.lock' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->initLockfilePath( nix::absPath( strPath ) ); } );
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Generate exception handling boilerplate for
 *        `EnvironmentMixin::init<MEMBER>' functions.
 */
#define ENV_MIXIN_THROW_IF_SET( member )                               \
  if ( this->member.has_value() )                                      \
    {                                                                  \
      throw EnvironmentMixinException( "`" #member                     \
                                       "' was already initializaed" ); \
    }                                                                  \
  if ( this->environment.has_value() )                                 \
    {                                                                  \
      throw EnvironmentMixinException(                                 \
        "`" #member "' cannot be initializaed after `environment'" );  \
    }


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initGlobalManifestPath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( globalManifestPath )
  this->globalManifestPath = std::move( path );
}

void
EnvironmentMixin::initGlobalManifest( GlobalManifest manifest )
{
  ENV_MIXIN_THROW_IF_SET( globalManifest )
  this->globalManifest = std::move( manifest );
}


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initManifestPath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( manifestPath )
  this->manifestPath = std::move( path );
}

void
EnvironmentMixin::initManifest( Manifest manifest )
{
  ENV_MIXIN_THROW_IF_SET( manifest )
  this->manifest = std::move( manifest );
}


/* -------------------------------------------------------------------------- */

void
EnvironmentMixin::initLockfilePath( std::filesystem::path path )
{
  ENV_MIXIN_THROW_IF_SET( lockfilePath )
  this->lockfilePath = std::move( path );
}

void
EnvironmentMixin::initLockfile( Lockfile lockfile )
{
  ENV_MIXIN_THROW_IF_SET( lockfile )
  this->lockfile = std::move( lockfile );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
