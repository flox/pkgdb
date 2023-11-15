/* ========================================================================== *
 *
 * @file flox/resolver/manifest.hh
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nix/config.hh>
#include <nix/globals.hh>
#include <nix/ref.hh>

#include "flox/core/nix-state.hh"
#include "flox/core/types.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"
#include "flox/resolver/manifest-raw.hh"


/* -------------------------------------------------------------------------- */

/* Forward Declarations. */

namespace flox::resolver {
struct ManifestDescriptor;
}  // namespace flox::resolver

namespace nix {
class Store;
}


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief A _global_ manifest containing only `registry` and `options` fields.
 *
 * This is intended for use outside of any particular project to supply inputs
 * for `flox search`, `flox show`, and similar commands.
 *
 * In the context of a project this file may be referenced, but its contents
 * will always yield priority to the project's own manifest, and in cases where
 * settings or inputs are not declared in a project, they may be automatically
 * added from the global manifest.
 */
class GlobalManifest
{

protected:

  /* We need these `protected' so they can be set by `Manifest'. */
  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
  // TODO: remove `manifestPath'
  ManifestRaw manifestRaw;
  RegistryRaw registryRaw;
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)


  /**
   * @brief Initialize @a registryRaw from @a manifestRaw. */
  void
  initRegistry();


public:

  virtual ~GlobalManifest()                = default;
  GlobalManifest()                         = default;
  GlobalManifest( const GlobalManifest & ) = default;
  GlobalManifest( GlobalManifest && )      = default;

  explicit GlobalManifest( GlobalManifestRaw raw )
    : manifestRaw( std::move( raw ) )
  {
    this->initRegistry();
  }

  explicit GlobalManifest( ManifestRaw raw ) : manifestRaw( std::move( raw ) )
  {
    this->initRegistry();
  }

  explicit GlobalManifest( std::filesystem::path manifestPath );

  GlobalManifest &
  operator=( const GlobalManifest & )
    = default;

  GlobalManifest &
  operator=( GlobalManifest && )
    = default;

  [[nodiscard]] const ManifestRaw &
  getManifestRaw() const
  {
    return this->manifestRaw;
  }

  [[nodiscard]] const RegistryRaw &
  getRegistryRaw() const
  {
    return this->registryRaw;
  }

  /* Ignore linter warning about copying params because `nix::ref` is just
   * a pointer ( `std::shared_pointer' with a `nullptr` check ). */
  // NOLINTBEGIN(performance-unnecessary-value-param)
  [[nodiscard]] RegistryRaw
  getLockedRegistry( nix::ref<nix::Store> store
                     = NixStoreMixin().getStore() ) const
  {
    return lockRegistry( this->getRegistryRaw(), store );
  }
  // NOLINTEND(performance-unnecessary-value-param)

  /** @brief Get the list of systems requested by the manifest. */
  [[nodiscard]] std::vector<System>
  getSystems() const
  {
    const auto & manifest = this->getManifestRaw();
    if ( manifest.options.has_value() && manifest.options->systems.has_value() )
      {
        return *manifest.options->systems;
      }
    return std::vector<System> { nix::settings.thisSystem.get() };
  }

  [[nodiscard]] pkgdb::PkgQueryArgs
  getBaseQueryArgs() const;


}; /* End class `GlobalManifest' */


/* -------------------------------------------------------------------------- */

/** @brief A map of _install IDs_ to _manifest descriptors_. */
using InstallDescriptors = std::unordered_map<InstallID, ManifestDescriptor>;


/** @brief Description of an environment in its _unlocked_ form. */
class Manifest : public GlobalManifest
{

private:

  /**
   * A map of _install ID_ to _descriptors_, being descriptions/requirements
   * of a dependency.
   */
  InstallDescriptors descriptors;


  /**
   * @brief Assert the validity of the manifest, throwing an exception if it
   *        contains invalid fields.
   *
   * This checks that:
   * - The raw manifest is valid.
   * - If `install.<IID>.systems` is set, then `options.systems` is also set.
   * - All `install.<IID>.systems` are in `options.systems`.
   */
  void
  check() const;

  /**
   * @brief Initialize @a descriptors from @a manifestRaw.
   */
  void
  initDescriptors();


public:

  ~Manifest() override         = default;
  Manifest()                   = default;
  Manifest( const Manifest & ) = default;
  Manifest( Manifest && )      = default;

  explicit Manifest( ManifestRaw raw ) : GlobalManifest( std::move( raw ) )
  {
    this->initDescriptors();
  }

  explicit Manifest( std::filesystem::path manifestPath );

  Manifest &
  operator=( const Manifest & )
    = default;

  Manifest &
  operator=( Manifest && )
    = default;

  /** @brief Get _descriptors_ from the manifest's `install' field. */
  [[nodiscard]] const InstallDescriptors &
  getDescriptors() const
  {
    return this->descriptors;
  }

  /**
   * @brief Returns all descriptors, grouping those with a _group_ field, and
   *        returning those without a group field as a map with a
   *        single element.
   */
  [[nodiscard]] std::vector<InstallDescriptors>
  getGroupedDescriptors() const;


}; /* End class `Manifest' */


/* -------------------------------------------------------------------------- */


}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
