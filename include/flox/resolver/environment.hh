/* ========================================================================== *
 *
 * @file flox/resolver/environment.hh
 *
 * @brief A collection of files associated with an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nix/ref.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/nix-state.hh"
#include "flox/core/types.hh"
#include "flox/pkgdb/input.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"
#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

/* Forward Declarations */

namespace argparse {
class Argument;
class ArgumentParser;
}  // namespace argparse

namespace flox {
namespace pkgdb {
class PkgDbReadOnly;
}
namespace resolver {
struct ManifestDescriptor;
}
}  // namespace flox


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
  lockPackage( const LockedInputRaw & input,
               pkgdb::PkgDbReadOnly & dbRO,
               pkgdb::row_id          row,
               unsigned               priority );

  static inline LockedPackageRaw
  lockPackage( const pkgdb::PkgDbInput & input,
               pkgdb::row_id             row,
               unsigned                  priority )
  {
    return lockPackage( LockedInputRaw( input ),
                        *input.getDbReadOnly(),
                        row,
                        priority );
  }

  /**
   * @brief Check if lock from @ oldLockfile can be reused for a group.
   *
   * Checks if:
   * - All descriptors are present in the old manifest.
   * - No descriptors have changed in the old manifest such that the lock is
   *   invalidated.
   * - All descriptors are present in the old lock
   */
  [[nodiscard]] bool
  groupIsLocked( const InstallDescriptors & group,
                 const Lockfile &           oldLockfile,
                 const System &             system );

  /**
   * @brief Get groups that need to be locked as opposed to reusing locks from
   * @a oldLockfile.
   */
  [[nodiscard]] std::vector<InstallDescriptors>
  getUnlockedGroups( const System & system );

  /**
   * @brief Get groups with locks that can be reused from @a oldLockfile.
   */
  [[nodiscard]] std::vector<InstallDescriptors>
  getLockedGroups( const System & system );

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

  /** @brief Try to resolve a descriptor in a given package database. */
  [[nodiscard]] std::optional<pkgdb::row_id>
  tryResolveDescriptorIn( const ManifestDescriptor & descriptor,
                          const pkgdb::PkgDbInput &  input,
                          const System &             system );

  /**
   * @brief Try to resolve a group of descriptors in a given package database.
   *
   * @return `std::nullopt` if resolution fails, otherwise a set of
   *          resolved packages for.
   */
  [[nodiscard]] std::optional<SystemPackages>
  tryResolveGroupIn( const InstallDescriptors & group,
                     const pkgdb::PkgDbInput &  input,
                     const System &             system );

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
  lockSystem( const System & system );


public:

  Environment( std::optional<GlobalManifest> globalManifest,
               Manifest                      manifest,
               std::optional<Lockfile>       oldLockfile )
    : globalManifest( std::move( globalManifest ) )
    , manifest( std::move( manifest ) )
    , oldLockfile( std::move( oldLockfile ) )
  {}

  explicit Environment( Manifest                manifest,
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
    const auto & global = this->getGlobalManifest();
    if ( ! global.has_value() ) { return std::nullopt; }
    const ManifestRaw & raw = global->getManifestRaw();
    return GlobalManifestRaw( raw.registry, raw.options );
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

  /**
   * @brief Get a base set of @a flox::pkgdb::PkgQueryArgs from
   *        combined options.
   */
  [[nodiscard]] const pkgdb::PkgQueryArgs &
  getCombinedBaseQueryArgs();

  /** @brief Get the set of supported systems. */
  [[nodiscard]] std::vector<System>
  getSystems() const
  {
    return this->getManifest().getSystems();
  }

  /** @brief Lazily initialize and get the combined registry's DBs. */
  [[nodiscard]] nix::ref<Registry<pkgdb::PkgDbInputFactory>>
  getPkgDbRegistry();

  // TODO: (Question) Should we lock the combined options and fill registry
  //                  `default` fields in inputs?
  /** @brief Create a new lockfile from @a manifest. */
  [[nodiscard]] Lockfile
  createLockfile();


}; /* End class `Environment' */


/* -------------------------------------------------------------------------- */

/**
 * @a class flox::resolver::EnvironmentMixinException
 * @brief An exception thrown by @a flox::resolver::EnvironmentMixin during
 *        its initialization.
 * @{
 */
FLOX_DEFINE_EXCEPTION( EnvironmentMixinException,
                       EC_ENVIRONMENT_MIXIN,
                       "EnvironmentMixin" )
/** @} */


/* -------------------------------------------------------------------------- */

/**
 * @brief A state blob with files associated with an environment.
 *
 * This structure stashes several fields to avoid repeatedly calculating them.
 */
class EnvironmentMixin
{

private:

  /* All member variables are calculated lazily using `std::optional' and
   * `get<MEMBER>' accessors.
   * Even for internal access you should use the `get<MEMBER>' accessors to
   * lazily initialize. */

  /** Path to user level manifest ( if any ). */
  std::optional<std::filesystem::path> globalManifestPath;
  /**
   * Contents of user level manifest with global registry and settings
   * ( if any ).
   */
  std::optional<GlobalManifest> globalManifest;

  /** Path to project level manifest. ( required ) */
  std::optional<std::filesystem::path> manifestPath;
  /**
   * Contents of project level manifest with registry, settings,
   * activation hook, and list of packages. ( required )
   */
  std::optional<Manifest> manifest;

  /** Path to project's lockfile ( if any ). */
  std::optional<std::filesystem::path> lockfilePath;
  /** Contents of project's lockfile ( if any ). */
  std::optional<Lockfile> lockfile;

  /** Lazily initialized environment wrapper. */
  std::optional<Environment> environment;


protected:

  /**
   * @brief Initialize the @a globalManifestPath member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initGlobalManifestPath( std::filesystem::path path );

  /**
   * @brief Initialize the @a globalManifest member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initGlobalManifest( GlobalManifest manifest );

  /**
   * @brief Initialize the @a manifestPath member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initManifestPath( std::filesystem::path path );

  /**
   * @brief Initialize the @a manifest member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initManifest( Manifest manifest );

  /**
   * @brief Initialize the @a lockfilePath member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initLockfilePath( std::filesystem::path path );

  /**
   * @brief Initialize the @a lockfile member variable.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initLockfile( Lockfile lockfile );


public:

  /**
   * @brief Lazily initialize and return the @a globalManifest.
   *
   * If @a globalManifest is set simply return it.
   * If @a globalManifest is unset, but @a globalManifestPath is set then
   * load from the file.
   */
  [[nodiscard]] const std::optional<GlobalManifest> &
  getGlobalManifest();

  /**
   * @brief Lazily initialize and return the @a manifest.
   *
   * If @a manifest is set simply return it.
   * If @a manifest is unset, but @a manifestPath is set then
   * load from the file.
   */
  [[nodiscard]] const Manifest &
  getManifest();

  /**
   * @brief Lazily initialize and return the @a lockfile.
   *
   * If @a lockfile is set simply return it.
   * If @a lockfile is unset, but @a lockfilePath is set then
   * load from the file.
   */
  [[nodiscard]] const std::optional<Lockfile> &
  getLockfile();

  /**
   * @brief Laziliy initialize and return the @a environment.
   *
   * The member variable @a manifest or @a manifestPath must be set for
   * initialization to succeed.
   * Member variables associated with the _global manifest_ and _lockfile_
   * are optional.
   *
   * After @a getEnvironment() has been called once, it is no longer possible
   * to use any `init*( MEMBER )` functions.
   */
  [[nodiscard]] Environment &
  getEnvironment();

  /**
   * @brief Sets the path to the global manifest file to load
   *        with `--global-manifest`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addGlobalManifestFileOption( argparse::ArgumentParser & parser );

  /**
   * @brief Sets the path to the manifest file to load with `--manifest`.
   * @param parser The parser to add the argument to.
   * @param required Whether the argument is required.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileOption( argparse::ArgumentParser & parser );

  /**
   * @brief Sets the path to the manifest file to load with a positional arg.
   * @param parser The parser to add the argument to.
   * @param required Whether the argument is required.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileArg( argparse::ArgumentParser & parser, bool required = true );

  /**
   * @brief Sets the path to the old lockfile to load with `--lockfile`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addLockfileOption( argparse::ArgumentParser & parser );


}; /* End class `EnvironmentMixin' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
