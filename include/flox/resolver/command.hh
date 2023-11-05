/* ========================================================================== *
 *
 * @file flox/resolver/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/resolver/manifest.hh"
#include "flox/resolver/params.hh"
#include "flox/resolver/state.hh"
#include "flox/search/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a set of package requirements to a set of
 *        satisfactory installables.
 */
class ResolveCommand : pkgdb::PkgDbRegistryMixin
{

private:

  ResolveOneParams       params; /**< Query arguments and inputs */
  command::VerboseParser parser; /**< Query arguments and inputs parser */

  /**
   * @brief Add argument to any parser to construct
   *        a @a flox::resolver::ResolveOneParams.
   */
  argparse::Argument &
  addResolveParamArgs( argparse::ArgumentParser & parser );

  [[nodiscard]] ResolverState
  getResolverState() const;

  [[nodiscard]] PkgDescriptorRaw
  getQuery() const
  {
    return this->params.query;
  }

  [[nodiscard]] RegistryRaw
  getRegistryRaw() override
  {
    return this->params.registry;
  }

  [[nodiscard]] const std::vector<std::string> &
  getSystems() override
  {
    return this->params.systems;
  }


public:

  ResolveCommand();

  [[nodiscard]] command::VerboseParser &
  getParser()
  {
    return this->parser;
  }

  /**
   * @brief Execute the `resolve` routine.
   *
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int
  run();


}; /* End class `ResolveCommand' */


/* -------------------------------------------------------------------------- */

/** @brief Lock a manifest file. */
class LockCommand : ManifestFileMixin
{

private:

  command::VerboseParser parser;


public:

  LockCommand();

  [[nodiscard]] command::VerboseParser &
  getParser()
  {
    return this->parser;
  }

  /**
   * @brief Execute the `lock` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int
  run();


}; /* End class `LockCommand' */


/* -------------------------------------------------------------------------- */

/** @brief Diff two manifest files. */
class DiffCommand
{

private:

  std::optional<std::filesystem::path> manifestPath;
  std::optional<ManifestRaw>           manifestRaw;

  std::optional<std::filesystem::path> oldManifestPath;
  std::optional<ManifestRaw>           oldManifestRaw;

  command::VerboseParser parser;


  [[nodiscard]] const ManifestRaw &
  getManifestRaw();

  [[nodiscard]] const ManifestRaw &
  getOldManifestRaw();


public:

  DiffCommand();

  [[nodiscard]] command::VerboseParser &
  getParser()
  {
    return this->parser;
  }

  /**
   * @brief Execute the `diff` routine.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int
  run();


}; /* End class `DiffCommand' */


/* -------------------------------------------------------------------------- */

class ManifestCommand
{

private:

  command::VerboseParser parser;  /**< `manifest`      parser */
  LockCommand            cmdLock; /**< `manifest lock` command */
  DiffCommand            cmdDiff; /**< `manifest diff` command */


public:

  ManifestCommand();

  [[nodiscard]] command::VerboseParser &
  getParser()
  {
    return this->parser;
  }

  /**
   * @brief Execute the `manifest` sub-command.
   * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
   */
  int
  run();


}; /* End class `ManifestCommand' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
