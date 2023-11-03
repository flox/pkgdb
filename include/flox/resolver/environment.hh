/* ========================================================================== *
 *
 * @file flox/resolver/environment.hh
 *
 * @brief A collection of files associated with an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>

#include "flox/core/types.hh"
#include "flox/core/util.hh"
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
class Environment : virtual private NixStoreMixin
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
  Lockfile lockfile;

  std::optional<RegistryRaw> combinedRegistryRaw;

  /** A registry of locked inputs. */
  std::optional<RegistryRaw> lockedRegistry;

  std::shared_ptr<pkgdb::PkgDbInputFactory> dbFactory;


public:

    [[nodiscard]] std::optional<GlobalManifest> &
    getGlobalManifest() const
    {
      return this->globalManifest;
    }

    [[nodiscard]] const Manifest &
    getManifest() const
    {
      return this->manifest;
    }

    [[nodiscard]] std::optional<Lockfile>
    getOldLockfile() const
    {
      return this->oldLockfile;
    }

    [[nodiscard]] const Lockfile &
    getLockfile() const
    {
      return this->lockfile;
    }

    // TODO
    [[nodiscard]] RegistryRaw &
    getCombinedRegistryRaw();


};  /* End class `Environment' */


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
  std::optional<LockfileRaw>           lockfileRaw;

  /** A registry of locked inputs. */
  std::optional<RegistryRaw> lockedRegistry;


public:

  /**
   * @brief Get a registry which is the combined contents of the
   *        _global_, _local_, and _lockfile_ registries.
   */
  RegistryRaw
  getCombinedRegistryRaw();

  /**
   * @brief Get a locked registry which is the combined contents of the
   *        _global_, _local_, and _lockfile_ registries.
   */
  const RegistryRaw &
  getLockedRegistry();


}; /* End class `EnvironmentMixin' */

/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
