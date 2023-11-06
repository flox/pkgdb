/* ========================================================================== *
 *
 * @file flox/resolver/manifest.hh
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/pkgdb/input.hh"
#include "flox/registry.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/resolver/resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @class flox::resolver::InvalidManifestFileException
 * @brief An exception thrown when a manifest file is invalid.
 * @{
 */
FLOX_DEFINE_EXCEPTION( InvalidManifestFileException,
                       EC_INVALID_MANIFEST_FILE,
                       "invalid manifest file" )
/** @} */


/* -------------------------------------------------------------------------- */

/**
 * @brief The `install.<INSTALL-ID>` field name associated with a package
 *        or descriptor.
 */
using InstallID = std::string;


/* -------------------------------------------------------------------------- */

/** @brief A set of options that apply to an entire environment. */
struct Options
{

  std::optional<std::vector<System>> systems;

  struct Allows
  {
    std::optional<bool>                     unfree;
    std::optional<bool>                     broken;
    std::optional<std::vector<std::string>> licenses;
  }; /* End struct `Allows' */
  std::optional<Allows> allow;

  struct Semver
  {
    std::optional<bool> preferPreReleases;
  }; /* End struct `Semver' */
  std::optional<Semver> semver;

  std::optional<std::string> packageGroupingStrategy;
  std::optional<std::string> activationStrategy;
  // TODO: Other options


  /**
   * @brief Apply options from @a overrides, but retain other existing options.
   */
  void
  merge( const Options & overrides );

  /** @brief Convert to a _base_ set of @a flox::pkgdb::PkgQueryArgs. */
  explicit operator pkgdb::PkgQueryArgs() const;


}; /* End struct `Options' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::Options. */
void
from_json( const nlohmann::json & jfrom, Options & opts );

/** @brief Convert a @a flox::resolver::Options to a JSON Object. */
void
to_json( nlohmann::json & jfrom, const Options & opts );


/* -------------------------------------------------------------------------- */

/**
 * @brief A _global_ manifest containing only `registry` and `options` fields
 *        in its _raw_ form.
 *
 * This _raw_ struct is defined to generate parsers, and its declarations simply
 * represent what is considered _valid_.
 * On its own, it performs no real work, other than to validate the input.
 *
 * @see flox::resolver::GlobalManifest
 */
struct GlobalManifestRaw
{
  std::optional<Options> options;

  std::optional<RegistryRaw> registry;


  virtual ~GlobalManifestRaw()                   = default;
  GlobalManifestRaw()                            = default;
  GlobalManifestRaw( const GlobalManifestRaw & ) = default;
  GlobalManifestRaw( GlobalManifestRaw && )      = default;

  GlobalManifestRaw &
  operator=( const GlobalManifestRaw & )
    = default;
  GlobalManifestRaw &
  operator=( GlobalManifestRaw && )
    = default;


  /**
   * @brief Validate manifest fields, throwing an exception if its contents
   *        are invalid.
   */
  virtual void
  check() const
  {}

  virtual void
  clear()
  {
    this->options  = std::nullopt;
    this->registry = std::nullopt;
  }


}; /* End struct `GlobalManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::GlobalManifest. */
void
from_json( const nlohmann::json & jfrom, GlobalManifestRaw & raw );

/** @brief Convert a @a flox::resolver::GlobalManifest to a JSON object. */
void
to_json( nlohmann::json & jto, const GlobalManifestRaw & raw );


/* -------------------------------------------------------------------------- */

/**
 * @brief A _raw_ description of an environment to be read from a file.
 *
 * This _raw_ struct is defined to generate parsers, and its declarations simply
 * represent what is considered _valid_.
 * On its own, it performs no real work, other than to validate the input.
 *
 * @see flox::resolver::Manifest
 */
struct ManifestRaw : public GlobalManifestRaw
{

  struct EnvBase
  {
    std::optional<std::string> floxhub;
    std::optional<std::string> dir;

    /**
     * @brief Validate the `env-base` field, throwing an exception if invalid
     *        information is found.
     *
     * This asserts:
     * - Only one of `floxhub` or `dir` is set.
     */
    void
    check() const;

    void
    clear()
    {
      this->floxhub = std::nullopt;
      this->dir     = std::nullopt;
    }
  }; /* End struct `EnvBase' */
  std::optional<EnvBase> envBase;

  std::optional<
    std::unordered_map<InstallID, std::optional<ManifestDescriptorRaw>>>
    install;

  std::optional<std::unordered_map<std::string, std::string>> vars;

  struct Hook
  {
    std::optional<std::string> script;
    std::optional<std::string> file;

    /**
     * @brief Validate `Hook` fields, throwing an exception if its contents
     *        are invalid.
     */
    void
    check() const;
  }; /* End struct `ManifestRaw::Hook' */
  std::optional<Hook> hook;

  ~ManifestRaw() override            = default;
  ManifestRaw()                      = default;
  ManifestRaw( const ManifestRaw & ) = default;
  ManifestRaw( ManifestRaw && )      = default;

  explicit ManifestRaw( const GlobalManifestRaw & globalManifestRaw )
    : GlobalManifestRaw( globalManifestRaw )
  {}

  explicit ManifestRaw( GlobalManifestRaw && globalManifestRaw )
    : GlobalManifestRaw( globalManifestRaw )
  {}

  ManifestRaw &
  operator=( const ManifestRaw & )
    = default;
  ManifestRaw &
  operator=( ManifestRaw && )
    = default;

  ManifestRaw &
  operator=( const GlobalManifestRaw & globalManifestRaw )
  {
    GlobalManifestRaw::operator=( globalManifestRaw );
    return *this;
  }
  ManifestRaw &
  operator=( GlobalManifestRaw && globalManifestRaw )
  {
    GlobalManifestRaw::operator=( globalManifestRaw );
    return *this;
  }

  /**
   * @brief Validate manifest fields, throwing an exception if its contents
   *        are invalid.
   *
   * This asserts:
   * - @a envBase is valid.
   * - @a registry does not contain indirect flake references.
   * - All members of @a install are valid.
   * - @a hook is valid.
   */
  void
  check() const override;

  void
  clear() override
  {
    /* From `GlobalManifestRaw' */
    this->options  = std::nullopt;
    this->registry = std::nullopt;
    /* From `ManifestRaw' */
    this->envBase = std::nullopt;
    this->install = std::nullopt;
    this->vars    = std::nullopt;
    this->hook    = std::nullopt;
  }

  /**
   * @brief Generate a JSON _diff_ between @a this manifest an @a old manifest.
   *
   * The _diff_ is represented as an [JSON patch](https://jsonpatch.com) object.
   */
  nlohmann::json
  diff( const ManifestRaw & old ) const;


}; /* End struct `ManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ManifestRaw. */
void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest );

/** @brief Convert a @a flox::resolver::ManifestRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const ManifestRaw & manifest );


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
  std::filesystem::path manifestPath;
  ManifestRaw           manifestRaw;
  RegistryRaw           registryRaw;
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)


  /**
   * @brief Initialize @a registryRaw from @a manifestRaw. */
  virtual void
  init();


public:

  ~GlobalManifest()                        = default;
  GlobalManifest()                         = default;
  GlobalManifest( const GlobalManifest & ) = default;
  GlobalManifest( GlobalManifest && )      = default;

  GlobalManifest( std::filesystem::path manifestPath, GlobalManifestRaw raw )
    : manifestPath( std::move( manifestPath ) ), manifestRaw( std::move( raw ) )
  {
    this->manifestRaw.check();
    if ( this->manifestRaw.registry.has_value() )
      {
        this->registryRaw = *this->manifestRaw.registry;
      }
  }

  explicit GlobalManifest( std::filesystem::path manifestPath );

  GlobalManifest &
  operator=( const GlobalManifest & )
    = default;

  GlobalManifest &
  operator=( GlobalManifest && )
    = default;


  [[nodiscard]] std::filesystem::path
  getManifestPath() const
  {
    return this->manifestPath;
  }

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
    return this->getManifestRaw().options->systems.value_or(
      std::vector<System> { nix::settings.thisSystem.get() } );
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
   * @brief Initialize @a registryRaw and @a descriptors from @a manifestRaw.
   */
  void
  init() override;


public:

  ~Manifest()                  = default;
  Manifest()                   = default;
  Manifest( const Manifest & ) = default;
  Manifest( Manifest && )      = default;

  Manifest( std::filesystem::path manifestPath, ManifestRaw raw );
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

  /** @brief Organize a set of descriptors by their _group_ field. */
  [[nodiscard]] std::unordered_map<GroupName, InstallDescriptors>
  getGroupedDescriptors() const;

  /** @brief Get descriptors which are not part of a group. */
  [[nodiscard]] InstallDescriptors
  getUngroupedDescriptors() const;


}; /* End class `Manifest' */


/* -------------------------------------------------------------------------- */

// TODO: class GlobalManifestMixin : public pkgdb::PkgDbRegistryMixin


/* -------------------------------------------------------------------------- */


}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
