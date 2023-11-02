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

#include "flox/pkgdb/input.hh"
#include "flox/registry.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/resolver/resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @class flox::resolver::InvalidManifestFileException
 * @brief An exception thrown when the value of manifestPath is invalid.
 * @{
 */
FLOX_DEFINE_EXCEPTION( InvalidManifestFileException,
                       EC_INVALID_MANIFEST_FILE,
                       "invalid manifest file" )
/** @} */


/* -------------------------------------------------------------------------- */

/**
 * @brief A _raw_ description of an environment to be read from a file.
 *
 * This _raw_ struct is defined to generate parsers, and its declarations simply
 * represent what is considered _valid_.
 * On its own, it performs no real work, other than to validate the input.
 */
struct ManifestRaw
{

  struct EnvBase
  {
    std::optional<std::string> floxhub;
    std::optional<std::string> dir;

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

  struct Options
  {

    std::optional<std::vector<std::string>> systems;

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

  }; /* End struct `Options' */
  std::optional<Options> options;

  std::optional<
    std::unordered_map<std::string, std::optional<ManifestDescriptorRaw>>>
    install;

  std::optional<RegistryRaw> registry;

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

  void
  clear()
  {
    this->envBase  = std::nullopt;
    this->options  = std::nullopt;
    this->install  = std::nullopt;
    this->registry = std::nullopt;
    this->vars     = std::nullopt;
    this->hook     = std::nullopt;
  }

}; /* End struct `ManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ManifestRaw. */
void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest );

/** @brief Convert a @a flox::resolver::ManifestRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const ManifestRaw & manifest );


/* -------------------------------------------------------------------------- */

class UnlockedManifest
{

private:

  std::filesystem::path                               manifestPath;
  ManifestRaw                                         manifestRaw;
  RegistryRaw                                         registryRaw;
  std::unordered_map<std::string, ManifestDescriptor> descriptors;

  void
  initDescriptors();


public:

  ~UnlockedManifest()                          = default;
  UnlockedManifest()                           = default;
  UnlockedManifest( const UnlockedManifest & ) = default;
  UnlockedManifest( UnlockedManifest && )      = default;

  UnlockedManifest( std::filesystem::path manifestPath, ManifestRaw raw )
    : manifestPath( std::move( manifestPath ) ), manifestRaw( std::move( raw ) )
  {
    if ( this->manifestRaw.registry.has_value() )
      {
        this->registryRaw = *this->manifestRaw.registry;
      }
    this->initDescriptors();
  }

  explicit UnlockedManifest( std::filesystem::path manifestPath );


  UnlockedManifest &
  operator=( const UnlockedManifest & )
    = default;

  UnlockedManifest &
  operator=( UnlockedManifest && )
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

  [[nodiscard]] RegistryRaw
  getLockedRegistry( nix::ref<nix::Store> store
                     = NixStoreMixin().getStore() ) const
  {
    return lockRegistry( this->getRegistryRaw(), store );
  }

  [[nodiscard]] pkgdb::PkgQueryArgs
  getBaseQueryArgs() const;

  [[nodiscard]] const std::unordered_map<std::string, ManifestDescriptor> &
  getDescriptors() const
  {
    return this->descriptors;
  }


}; /* End class `UnlockedManifest' */


/* -------------------------------------------------------------------------- */

/**
 * @brief A state blob with a manifest loaded from path.
 *
 * This structure stashes several fields to avoid repeatedly calculating them.
 */
struct ManifestFileMixin : public pkgdb::PkgDbRegistryMixin
{

public:

  /* From `PkgDbRegistryMixin':
   *   std::shared_ptr<nix::Store>                         store;
   *   std::shared_ptr<nix::EvalState>                     state;
   *   bool                                                force    = false;
   *   std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;
   */

  std::optional<std::filesystem::path> manifestPath;
  std::optional<UnlockedManifest>      manifest;
  std::optional<RegistryRaw>           lockedRegistry;
  std::optional<pkgdb::PkgQueryArgs>   baseQueryArgs;

  std::unordered_map<
    std::string,                                       /* group name */
    std::unordered_map<std::string,                    /* input name */
                       std::unordered_map<std::string, /* _install ID_ */
                                          std::optional<pkgdb::row_id>>>>
    groupedResolutions;

  /**
   * @brief A map of _locked_ descriptors organized by their _install ID_,
   *        and then by `system`.
   *        For optional packages, or those which are explicitly declared for
   *        a subset of systems, the value may be `std::nullopt`.
   */
  std::unordered_map<std::string,                    /* _install ID_ */
                     std::unordered_map<std::string, /* system */
                                        std::optional<Resolved>>>
    lockedDescriptors;


protected:

  const std::unordered_map<std::string, std::optional<Resolved>> &
  lockUngroupedDescriptor( const std::string &        iid,
                           const ManifestDescriptor & desc );

  /** @brief Assert that all _grouped_ descriptors resolve to a single input. */
  void
  checkGroups();


public:

  /**
   * @brief Returns the locked @a RegistryRaw from the manifest.
   *
   * This is used to initialize the @a registry field from
   * @a flox::pkgdb::PkgDbRegistryMixin and should not be confused with the
   * _unlocked registry_ ( which can be accessed directly from @a manifest ).
   */
  [[nodiscard]] RegistryRaw
  getRegistryRaw() override
  {
    return this->getUnlockedManifest().getRegistryRaw();
  }

  [[nodiscard]] const std::vector<std::string> &
  getSystems() override
  {
    return this->getBaseQueryArgs().systems;
  }

  /**
   * @brief Get the path to the manifest file.
   *
   * If @a manifestPath is already set, we use that; otherwise we attempt to
   * locate a manifest at `[./flox/]manifest.{toml,yaml,json}`.
   *
   * @return The path to the manifest file.
   */
  [[nodiscard]] std::filesystem::path
  getManifestPath();

  /**
   * @brief Sets the path to the registry file to load with `--manifest`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileOption( argparse::ArgumentParser & parser );

  /**
   * @brief Sets the path to the registry file to load with a positional arg.
   * @param parser The parser to add the argument to.
   * @param required Whether the argument is required.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addManifestFileArg( argparse::ArgumentParser & parser, bool required = true );

  [[nodiscard]] const UnlockedManifest &
  getUnlockedManifest();

  [[nodiscard]] const RegistryRaw &
  getLockedRegistry();

  [[nodiscard]] const pkgdb::PkgQueryArgs &
  getBaseQueryArgs();

  [[nodiscard]] const std::unordered_map<std::string, ManifestDescriptor> &
  getDescriptors()
  {
    return this->getUnlockedManifest().getDescriptors();
  }

  [[nodiscard]] const std::unordered_map<
    std::string,
    std::unordered_map<std::string, std::optional<Resolved>>> &
  getLockedDescriptors();


}; /* End struct `ManifestFileMixin' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
