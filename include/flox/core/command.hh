/* ========================================================================== *
 *
 * @file flox/core/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <argparse/argparse.hpp>

#include "flox/core/nix-state.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/flox-flake.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

/** @brief Executable command helpers, argument parsers, etc. */
namespace flox::command {

/* -------------------------------------------------------------------------- */

/**
 * @brief Add verbosity flags to any parser and modify the global verbosity.
 *
 * Nix verbosity levels for reference ( we have no `--debug` flag ):
 *   typedef enum {
 *     lvlError = 0   ( --quiet --quiet --quiet )
 *   , lvlWarn        ( --quiet --quiet )
 *   , lvlNotice      ( --quiet )
 *   , lvlInfo        ( **Default** )
 *   , lvlTalkative   ( -v )
 *   , lvlChatty      ( -vv   | --debug --quiet )
 *   , lvlDebug       ( -vvv  | --debug )
 *   , lvlVomit       ( -vvvv | --debug -v )
 *   } Verbosity;
 */
struct VerboseParser : public argparse::ArgumentParser
{
  explicit VerboseParser( const std::string & name,
                          const std::string & version = "0.1.0" );
}; /* End struct `VerboseParser' */


/* -------------------------------------------------------------------------- */

/** @brief Extend a command's state blob with a single @a RegistryInput. */
class InlineInputMixin : virtual public NixState
{

private:

  RegistryInput registryInput;

protected:

  /**
   * @brief Fill @a registryInput by parsing a flake ref.
   * @param flakeRef A flake reference as a URL string or JSON attribute set.
   */
  void
  parseFlakeRef( const std::string & flakeRef )
  {
    this->registryInput.from
      = std::make_shared<nix::FlakeRef>( flox::parseFlakeRef( flakeRef ) );
  }


public:

  argparse::Argument &
  addSubtreeArg( argparse::ArgumentParser & parser );
  argparse::Argument &
  addStabilityArg( argparse::ArgumentParser & parser );
  argparse::Argument &
  addFlakeRefArg( argparse::ArgumentParser & parser );

  /**
   * @brief Return the parsed @a RegistryInput.
   * @return The parsed @a RegistryInput.
   */
  [[nodiscard]] const RegistryInput &
  getRegistryInput()
  {
    return this->registryInput;
  }

}; /* End struct `InlineInputMixin' */


/* -------------------------------------------------------------------------- */

/** @brief Extend a command state blob with an attribute path to "target". */
struct AttrPathMixin
{

  flox::AttrPath attrPath;

  /**
   * @brief Sets the attribute path to be scraped.
   *
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable".
   */
  argparse::Argument &
  addAttrPathArgs( argparse::ArgumentParser & parser );

  /**
   * @brief Sets fallback `attrPath` to a package set.
   *
   * If `attrPath` is empty use, `packages.<SYTEM>`.
   * If `attrPath` is one element then add "current system" as `<SYSTEM>`.
   * If `attrPath` is a catalog with no stability use `stable`.
   */
  void
  fixupAttrPath();

}; /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

/** @brief Extend a command state blob with registry inputs loaded from "path".
 */
struct RegistryFileMixin
{

  std::optional<std::filesystem::path> registryPath;
  std::optional<RegistryRaw>           registryRaw;

protected:

  /**
   * @brief Loads the registry.
   *
   * Requires that the registry file is already set.
   *
   */
  void
  loadRegistry();

public:

  /**
   * @brief Sets the path to the registry file to load.
   *
   */
  argparse::Argument &
  addRegistryFileArg( argparse::ArgumentParser & parser );

  /**
   * @brief Sets the path to the registry file.
   *
   */
  void
  setRegistryPath( const std::filesystem::path & path );

  /**
   * @brief Returns the @a RegistryRaw from the provided file path.
   *
   */
  const RegistryRaw &
  getRegistryRaw();

}; /* End struct `RegistryFileMixin' */

/* -------------------------------------------------------------------------- */

}  // namespace flox::command


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
