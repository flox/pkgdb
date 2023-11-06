/* ========================================================================== *
 *
 * @file flox/resolver/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/resolver/environment.hh"
#include "flox/resolver/manifest.hh"
#include "flox/search/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/** @brief Lock a manifest file. */
class LockCommand : public EnvironmentMixin
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
