/* ========================================================================== *
 *
 * @file flox/resolver/mixins.hh
 *
 * @brief State blobs for flox commands.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

#include "flox/core/exceptions.hh"
#include "flox/resolver/environment.hh"
#include "flox/resolver/lockfile.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

/* Forward Declarations */

namespace argparse {

class Argument;
class ArgumentParser;

}  // namespace argparse


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

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
  virtual void
  initGlobalManifest( GlobalManifestRaw manifestRaw );

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
  virtual void
  initManifest( ManifestRaw manifestRaw );

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
  initLockfile( LockfileRaw lockfileRaw );

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

  /** @brief Get the filesystem path to the global manifest ( if any ). */
  [[nodiscard]] const std::optional<std::filesystem::path> &
  getGlobalManifestPath() const;

  /**
   * @brief Lazily initialize and return the @a globalManifest.
   *
   * If @a globalManifest is set simply return it.
   * If @a globalManifest is unset, but @a globalManifestPath is set then
   * load from the file.
   */
  [[nodiscard]] virtual const std::optional<GlobalManifest> &
  getGlobalManifest();

  /** @brief Get the filesystem path to the manifest ( if any ). */
  [[nodiscard]] const std::optional<std::filesystem::path> &
  getManifestPath() const;

  /**
   * @brief Lazily initialize and return the @a manifest.
   *
   * If @a manifest is set simply return it.
   * If @a manifest is unset, but @a manifestPath is set then
   * load from the file.
   */
  [[nodiscard]] virtual const Manifest &
  getManifest();

  /** @brief Get the filesystem path to the lockfile ( if any ). */
  [[nodiscard]] const std::optional<std::filesystem::path> &
  getLockfilePath() const;

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

  /**
   * @brief Uses a `--dir PATH` to locate `manifest.{toml,yaml,json}' file and
   *        `manifest.lock` if it is present.`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addFloxDirectoryOption( argparse::ArgumentParser & parser );


}; /* End class `EnvironmentMixin' */


/* -------------------------------------------------------------------------- */

class GAEnvironmentMixin : public EnvironmentMixin
{

private:

  /** Whether to override manifest registries for use with `flox` GA. */
  bool gaRegistry = false;


protected:

  /**
   * @brief Initialize the @a globalManifest member variable.
   *        This form enforces `--ga-registry` by disallowing `registry` in its
   *        input, and injecting a hard coded `registry`.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initGlobalManifest( GlobalManifestRaw manifestRaw ) override;

  /**
   * @brief Initialize the @a manifest member variable.
   *        This form enforces `--ga-registry` by disallowing `registry` in its
   *        input, and injecting a hard coded `registry`.
   *
   * This may only be called once and must be called before
   * `getEnvironment()` is ever used - otherwise an exception will be thrown.
   *
   * This function exists so that child classes can initialize their
   * @a flox::resolver::EnvirontMixin base at runtime without accessing
   * private member variables.
   */
  void
  initManifest( ManifestRaw manifestRaw ) override;


public:

  /**
   * @brief Hard codes a manifest with only `github:NixOS/nixpkgs/release-23.05`
   * with `--ga-registry`.
   * @param parser The parser to add the argument to.
   * @return The argument added to the parser.
   */
  argparse::Argument &
  addGARegistryOption( argparse::ArgumentParser & parser );


}; /* End class `GAEnvironmentMixin' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
