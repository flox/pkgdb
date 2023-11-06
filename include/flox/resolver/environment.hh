/* ========================================================================== *
 *
 * @file flox/resolver/environment.hh
 *
 * @brief A collection of files associated with an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <memory>
#include <optional>

#include <nix/ref.hh>

#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/input.hh"
#include "flox/registry.hh"
#include "flox/resolver/lockfile.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

FLOX_DEFINE_EXCEPTION( ResolutionFailure,
                       EC_RESOLUTION_FAILURE,
                       "resolution failure" );


/* -------------------------------------------------------------------------- */

/**
 * @brief A collection of data associated with an environment and its state.
 *
 * This structure provides a number of helper routines which require knowledge
 * of manifests and lockfiles together - most importantly, locking descriptors.
 *
 * @see flox::resolver::GlobalManifest
 * @see flox::resolver::Manifest
 * @see flox::resolver::Lockfile
 */
class Environment : private NixStoreMixin
{

private:

  /* From `NixStoreMixin':
   *   std::shared_ptr<nix::Store> store
   */

  /** Contents of user level manifest with global registry and settings. */
  std::optional<GlobalManifest> globalManifest;

  /** The environment manifest. */
  Manifest manifest;

  /** Previous generation of the lockfile ( if any ). */
  std::optional<Lockfile> oldLockfile;

  /** New/modified lockfile being edited. */
  std::optional<LockfileRaw> lockfileRaw;

  std::optional<RegistryRaw> combinedRegistryRaw;

  std::optional<Options> combinedOptions;

  std::optional<pkgdb::PkgQueryArgs> combinedBaseQueryArgs;

  /** A registry of locked inputs. */
  std::optional<RegistryRaw> lockedRegistry;

  std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> dbs;

  static LockedPackageRaw
  lockPackage( const LockedInputRaw &input,
               pkgdb::PkgDbReadOnly &dbRO,
               pkgdb::row_id         row,
               unsigned              priority );

  static inline LockedPackageRaw
  lockPackage( const pkgdb::PkgDbInput &input,
               pkgdb::row_id            row,
               unsigned                 priority )
  {
    return lockPackage( LockedInputRaw( input ),
                        *input.getDbReadOnly(),
                        row,
                        priority );
  }

  // TODO: Implement using JSON patch `diff` against old manifest.
  //       The current implementation is a hot mess.
  /**
   *  @brief Fill resolutions from @a oldLockfile into @a lockfile for
   *         unmodified descriptors.
   *         Drop any removed descriptors in the process.
   *         This is a helper function of
   *         @a flox::resolver::Environment::createLockfile().
   *
   * This must be called after @a lockfileRaw is initialized.
   * This is only intended to be called from
   * @a flox::resolver::Environment::createLockfile().
   */
  void
  fillLockedFromOldLockfile();

  /** @brief Lazily initialize and get the combined registry's DBs. */
  [[nodiscard]] nix::ref<Registry<pkgdb::PkgDbInputFactory>>
  getPkgDbRegistry();

  // TODO
  /**
   * @brief Get the set of descriptors which differ from those
   *        in @a oldLockfile.
   */
  /** @brief Collect unlocked/modified descriptors that need to be resolved. */
  [[nodiscard]] std::unordered_map<InstallID, ManifestDescriptor>
  getUnlockedDescriptors();

  /**
   * @brief Get a merged form of @a oldLockfile or @a globalManifest
   *        ( if available ) and @a manifest options.
   *
   * Global options have the lowest priority, and will be clobbered by
   * locked options.
   * Options defined in the current manifest have the highest priority and will
   * clobber all other settings.
   */
  [[nodiscard]] const Options &
  getCombinedOptions();

  /**
   * @brief Get a base set of @a flox::pkgdb::PkgQueryArgs from
   *        combined options.
   */
  [[nodiscard]] const pkgdb::PkgQueryArgs &
  getCombinedBaseQueryArgs();

  /** @brief Try to resolve a descriptor in a given package database. */
  [[nodiscard]] std::optional<pkgdb::row_id>
  tryResolveDescriptorIn( const ManifestDescriptor &descriptor,
                          const pkgdb::PkgDbInput  &input,
                          const System             &system );

  /**
   * @brief Try to resolve a group of descriptors in a given package database.
   *
   * @return `std::nullopt` if resolution fails, otherwise a set of
   *          resolved packages for.
   */
  [[nodiscard]] std::optional<SystemPackages>
  tryResolveGroupIn( const InstallDescriptors &group,
                     const pkgdb::PkgDbInput  &input,
                     const System             &system );

  // TODO: Only update changed descriptors.
  /**
   * @brief Lock all descriptors for a given system.
   *        This is a helper function of
   *        @a flox::resolver::Environment::createLockfile().
   *
   * This must be called after @a lockfileRaw is initialized.
   * This is only intended to be called from
   * @a flox::resolver::Environment::createLockfile().
   */
  void
  lockSystem( const System &system );


public:

  Environment( std::optional<GlobalManifest> globalManifest,
               Manifest                      manifest,
               std::optional<Lockfile>       oldLockfile )
    : globalManifest( std::move( globalManifest ) )
    , manifest( std::move( manifest ) )
    , oldLockfile( std::move( oldLockfile ) )
  {}

  Environment( Manifest                manifest,
               std::optional<Lockfile> oldLockfile = std::nullopt )
    : globalManifest( std::nullopt )
    , manifest( std::move( manifest ) )
    , oldLockfile( std::move( oldLockfile ) )
  {}

  [[nodiscard]] const std::optional<GlobalManifest> &
  getGlobalManifest() const
  {
    return this->globalManifest;
  }

  [[nodiscard]] std::optional<GlobalManifestRaw>
  getGlobalManifestRaw() const
  {
    if ( ! this->getGlobalManifest().has_value() ) { return std::nullopt; }
    return this->getGlobalManifest()->getManifestRaw();
  }

  [[nodiscard]] const Manifest &
  getManifest() const
  {
    return this->manifest;
  }

  [[nodiscard]] const ManifestRaw &
  getManifestRaw() const
  {
    return this->getManifest().getManifestRaw();
  }

  /** @brief Get the old manifest from @a oldLockfile if it exists. */
  [[nodiscard]] std::optional<ManifestRaw>
  getOldManifestRaw() const;

  [[nodiscard]] std::optional<Lockfile>
  getOldLockfile() const
  {
    return this->oldLockfile;
  }

  // TODO: Implement
  /**
   * @brief Get a merged form of @a oldLockfile ( if available ),
   *        @a globalManifest ( if available ) and @a manifest registries.
   *
   * The Global registry has the lowest priority, and will be clobbered by
   * locked registry inputs/settings.
   * The registry defined in the current manifest has the highest priority and
   * will clobber all other inputs/settings.
   */
  [[nodiscard]] RegistryRaw &
  getCombinedRegistryRaw();

  /** @brief Get the set of supported systems. */
  [[nodiscard]] std::vector<System>
  getSystems() const
  {
    return this->getManifest().getSystems();
  }

  // TODO: (Question) Should we lock the combined options and fill registry
  //                  `default` fields in inputs?
  /** @brief Create a new lockfile from @a manifest. */
  [[nodiscard]] Lockfile
  createLockfile();


}; /* End class `Environment' */


/* -------------------------------------------------------------------------- */

/**
 * @brief A state blob with files associated with an environment.
 *
 * This structure stashes several fields to avoid repeatedly calculating them.
 */
class EnvironmentMixin
{

private:

  /** Path to user level manifest. */
  std::optional<std::filesystem::path> globalManifestPath;
  /** Contents of user level manifest with global registry and settings. */
  std::optional<GlobalManifest> globalManifest;

  std::optional<std::filesystem::path> manifestPath;
  std::optional<Manifest>              manifest;

  std::optional<std::filesystem::path> lockfilePath;
  std::optional<Lockfile>              lockfile;

  std::optional<Environment> environment;


public:

  /**
   * @brief Lazily initialize and return the @a globalManifest
   *        if @a globalManifestPath is set.
   */
  [[nodiscard]] const std::optional<GlobalManifest> &
  getGlobalManifest();

  /** @brief Lazily initialize and return the @a manifest. */
  [[nodiscard]] const Manifest &
  getManifest();

  /**
   * @brief Lazily initialize and return the @a globalManifest
   *        if @a globalManifestPath is set.
   */
  [[nodiscard]] const std::optional<Lockfile> &
  getLockfile();

  /** @brief Laziliy initialize and return the @a environment. */
  [[nodiscard]] Environment &
  getEnvironment();

  /**
   * @brief Sets the path to the global manifest file to load
   *        with `--global-manifest`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addGlobalManifestFileOption( argparse::ArgumentParser &parser );

  /**
   * @brief Sets the path to the manifest file to load with `--manifest`.
   * @param parser The parser to add the argument to.
   * @param required Whether the argument is required.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileOption( argparse::ArgumentParser &parser );

  /**
   * @brief Sets the path to the manifest file to load with a positional arg.
   * @param parser The parser to add the argument to.
   * @param required Whether the argument is required.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileArg( argparse::ArgumentParser &parser, bool required = true );

  /**
   * @brief Sets the path to the old lockfile to load with `--lockfile`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addLockfileOption( argparse::ArgumentParser &parser );


}; /* End class `EnvironmentMixin' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
