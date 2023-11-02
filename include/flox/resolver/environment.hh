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
#include "flox/resolver/manifest.hh"
#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief A state blob with a manifest loaded from path.
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
  std::optional<UnlockedManifest>      globalManifest;

  std::optional<std::filesystem::path> manifestPath;
  std::optional<UnlockedManifest>      manifest;

  std::optional<std::filesystem::path> lockfilePath;
  std::optional<LockfileRaw>           lockfileRaw;

  /** A registry of locked inputs. */
  std::optional<RegistryRaw>           lockedRegistry;


public:

  /**
   * @brief Get a registry which is the combined contents of the
   *        _global_, _local_, and _lockfile_ registries.
   */
  const RegistryRaw &
  getCombinedRegistryRaw();

  /**
   * @brief Get a locked registry which is the combined contents of the
   *        _global_, _local_, and _lockfile_ registries.
   */
  const RegistryRaw &
  getLockedRegistry();


};  /* End class `EnvironmentMixin' */

/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
