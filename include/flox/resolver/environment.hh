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
  LockfileRaw lockfileRaw;

  std::optional<RegistryRaw> combinedRegistryRaw;

  std::optional<ManifestRaw::Options> combinedOptions;

  std::optional<pkgdb::PkgQueryArgs> combinedBaseQueryArgs;

  /** A registry of locked inputs. */
  std::optional<RegistryRaw> lockedRegistry;

  std::shared_ptr<pkgdb::PkgDbInputFactory> dbFactory;

  std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> dbs;


  // TODO
  /**
   *  @brief Fill resolutions from @a oldLockfile into @a lockfile for
   *         unmodified descriptors.
   *         Drop any removed descriptors in the process.
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

  // TODO
  /** @brief Get a merged form of @a globalManifest and @a manifest options. */
  [[nodiscard]] const ManifestRaw::Options &
  getCombinedOptions();

  // TODO
  /**
   * @brief Get a base set of @a flox::pkgdb::PkgQueryArgs from the combined
   *        registry, and combined options.
   */
  [[nodiscard]] const pkgdb::PkgQueryArgs &
  getCombinedBaseQueryArgs();

  // TODO
  /** @brief Try to resolve a descriptor in a given package database. */
  [[nodiscard]] std::optional<pkgdb::row_id>
  tryResolveDescriptorIn( const ManifestDescriptor & descriptor,
                          const pkgdb::PkgDbInput &  input,
                          const System &             system );

  // TODO
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

  // TODO
  /** @brief Lock all descriptors for a given system. */
  void
  lockSystem( const System & system );


public:

  [[nodiscard]] const std::optional<GlobalManifest> &
  getGlobalManifest() const
  {
    return this->globalManifest;
  }

  [[nodiscard]] const Manifest &
  getManifest() const
  {
    return this->manifest;
  }

  // TODO
  /** @brief Get the old manifest from @a oldLockfile if it exists. */
  [[nodiscard]] const std::optional<ManifestRaw> &
  getOldManifestRaw();

  [[nodiscard]] std::optional<Lockfile>
  getOldLockfile() const
  {
    return this->oldLockfile;
  }

  // TODO
  [[nodiscard]] RegistryRaw &
  getCombinedRegistryRaw();

  // TODO
  /** @brief Create a new lockfile from @a manifest. */
  [[nodiscard]] const Lockfile &
  createLockfile();


}; /* End class `Environment' */


/* -------------------------------------------------------------------------- */

/**
 * @brief A state blob with files associated with an environment.
 *
 * This structure stashes several fields to avoid repeatedly calculating them.
 */
class EnvironmentMixin : public pkgdb::PkgDbRegistryMixin
{

private:

  /* From `PkgDbRegistryMixin':
   *   std::shared_ptr<nix::Store>                         store;
   *   std::shared_ptr<nix::EvalState>                     state;
   *   bool                                                force    = false;
   *   std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;
   */

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

  // TODO
  /**
   * @brief Lazily initialize and return the @a globalManifest
   *        if @a globalManifestPath is set.
   */
  [[nodiscard]] const std::optional<GlobalManifest> &
  getGlobalManifest();

  // TODO
  /** @brief Lazily initialize and return the @a manifest. */
  [[nodiscard]] const GlobalManifest &
  getManifest();

  // TODO
  /**
   * @brief Lazily initialize and return the @a globalManifest
   *        if @a globalManifestPath is set.
   */
  [[nodiscard]] const std::optional<Lockfile> &
  getLockfile();

  // TODO
  /** @brief Laziliy initialize and return the @a environment. */
  [[nodiscard]] Environment
  getEnvironment();


}; /* End class `EnvironmentMixin' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
