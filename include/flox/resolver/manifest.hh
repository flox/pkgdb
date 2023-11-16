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

/** @brief Read a flox::resolver::ManifestBase from a file. */
template<manifest_raw_type RawType>
static inline RawType
readManifestFromPath( const std::filesystem::path & manifestPath )
{
  if ( ! std::filesystem::exists( manifestPath ) )
    {
      throw InvalidManifestFileException( "no such path: "
                                          + manifestPath.string() );
    }
  return readAndCoerceJSON( manifestPath );
}


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
template<manifest_raw_type RawType>
class ManifestBase
{

protected:

  /* We need these `protected' so they can be set by `Manifest'. */
  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
  // TODO: remove `manifestPath'
  RawType     manifestRaw;
  RegistryRaw registryRaw;
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)


public:

  using rawType = RawType;

  virtual ~ManifestBase()              = default;
  ManifestBase()                       = default;
  ManifestBase( const ManifestBase & ) = default;
  ManifestBase( ManifestBase && )      noexcept = default;

  explicit ManifestBase( RawType raw ) : manifestRaw( std::move( raw ) ) {}

  explicit ManifestBase( const std::filesystem::path & manifestPath )
    : manifestRaw( readManifestFromPath<RawType>( manifestPath ) )
  {}

  ManifestBase &
  operator=( const ManifestBase & )
    = default;

  ManifestBase &
  operator=( ManifestBase && ) noexcept
    = default;

  [[nodiscard]] const RawType &
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
  [[nodiscard]] RegistryRaw
  getLockedRegistry( const nix::ref<nix::Store> & store
                     = NixStoreMixin().getStore() ) const
  {
    return lockRegistry( this->getRegistryRaw(), store );
  }

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
  getBaseQueryArgs() const
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


}; /* End class `ManifestBase' */


/* -------------------------------------------------------------------------- */

class GlobalManifest : public ManifestBase<GlobalManifestRaw>
{

protected:

  /** @brief Initialize @a registryRaw from @a manifestRaw. */
  void
  initRegistry()
  {
    this->manifestRaw.check();
    if ( this->manifestRaw.registry.has_value() )
      {
        this->registryRaw = *this->manifestRaw.registry;
      }
  }


public:

  ~GlobalManifest() override               = default;
  GlobalManifest()                         = default;
  GlobalManifest( const GlobalManifest & ) = default;
  GlobalManifest( GlobalManifest && )      = default;

  GlobalManifest &
  operator=( const GlobalManifest & )
    = default;
  GlobalManifest &
  operator=( GlobalManifest && )
    = default;

  explicit GlobalManifest( GlobalManifestRaw raw )
    : ManifestBase<GlobalManifestRaw>( std::move( raw ) )
  {
    this->initRegistry();
  }

  explicit GlobalManifest( const std::filesystem::path & manifestPath )
    : ManifestBase<GlobalManifestRaw>( manifestPath )
  {
    this->initRegistry();
  }


}; /* End class `GlobalManifest' */


/* -------------------------------------------------------------------------- */

/** @brief A map of _install IDs_ to _manifest descriptors_. */
using InstallDescriptors = std::unordered_map<InstallID, ManifestDescriptor>;


/** @brief Description of an environment in its _unlocked_ form. */
class Manifest : public ManifestBase<ManifestRaw>
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

  /** @brief Initialize @a descriptors from @a manifestRaw. */
  void
  initDescriptors();


protected:

  /** @brief Initialize @a registryRaw from @a manifestRaw. */
  void
  initRegistry()
  {
    this->manifestRaw.check();
    if ( this->manifestRaw.registry.has_value() )
      {
        this->registryRaw = *this->manifestRaw.registry;
      }
  }


public:

  ~Manifest() override         = default;
  Manifest()                   = default;
  Manifest( const Manifest & ) = default;
  Manifest( Manifest && )      = default;

  explicit Manifest( ManifestRaw raw )
    : ManifestBase<ManifestRaw>( std::move( raw ) )
  {
    this->initRegistry();
    this->initDescriptors();
  }

  explicit Manifest( const std::filesystem::path & manifestPath )
    : ManifestBase<ManifestRaw>(
      readManifestFromPath<ManifestRaw>( manifestPath ) )
  {
    this->initRegistry();
    this->initDescriptors();
  }

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
