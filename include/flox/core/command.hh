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
#include <optional>
#include <memory>

#include <argparse/argparse.hpp>

#include "flox/core/types.hh"
#include "flox/core/nix-state.hh"
#include "flox/core/util.hh"
#include "flox/flox-flake.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

/** Executable command helpers, argument parsers, etc. */
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
struct VerboseParser : public argparse::ArgumentParser {
  explicit VerboseParser( const std::string & name
                        , const std::string & version = "0.1.0"
                        );
};  /* End struct `VerboseParser' */


/* -------------------------------------------------------------------------- */

/** Virtual class for _mixins_ which extend a command's state blob */
struct CommandStateMixin {

  constexpr CommandStateMixin()                             = default;
  constexpr CommandStateMixin( const CommandStateMixin &  ) = default;
  constexpr CommandStateMixin(       CommandStateMixin && ) = default;

  virtual ~CommandStateMixin() = default;

  CommandStateMixin & operator=( const CommandStateMixin &  ) = default;
  CommandStateMixin & operator=(       CommandStateMixin && ) = default;

  /** Hook run after parsing arguments and before running commands. */
  virtual void postProcessArgs() {};

};  /* End struct `CommandStateMixin' */


/* -------------------------------------------------------------------------- */

/** Extend a command's state blob with a @a flox::FloxFlake. */
struct FloxFlakeMixin
  : public CommandStateMixin
  , public FloxFlakeParserMixin
{

  std::shared_ptr<flox::FloxFlake> flake;

  /**
   * Populate the command state's @a flake with the a flake reference.
   * @param flakeRef A URI string or JSON representation of a flake reference.
   */
  void parseFloxFlake( const std::string & flakeRef );

  /** Extend an argument parser to accept a `flake-ref` argument. */
  argparse::Argument & addFlakeRefArg( argparse::ArgumentParser & parser );

};  /* End struct `FloxFlakeMixin' */


/* -------------------------------------------------------------------------- */

/** Extend a command's state blob with a single @a RegistryInput. */
struct InlineInputMixin
  :         public CommandStateMixin
  , virtual public NixState
{

  protected:

    RegistryInput registryInput;

    /**
     * @brief Fill @a registryInput by parsing a flake ref.
     * @param flakeRef A flake reference as a URL string or JSON attribute set.
     */
      void
    parseFlakeRef( const std::string & flakeRef )
    {
      this->registryInput.from = std::make_shared<nix::FlakeRef>(
        flox::parseFlakeRef( flakeRef )
      );
    }


  public:

    argparse::Argument & addSubtreeArg(   argparse::ArgumentParser & parser );
    argparse::Argument & addStabilityArg( argparse::ArgumentParser & parser );
    argparse::Argument & addFlakeRefArg(  argparse::ArgumentParser & parser );

    /**
     * @brief Return the parsed @a RegistryInput.
     * @return The parsed @a RegistryInput.
     */
    [[nodiscard]]
    const RegistryInput & getRegistryInput() { return this->registryInput; }

};  /* End struct `InlineInputMixin' */


/* -------------------------------------------------------------------------- */

/** Extend a command state blob with an attribute path to "target". */
struct AttrPathMixin : public CommandStateMixin {

  flox::AttrPath attrPath;

  /**
   * Sets the attribute path to be scraped.
   * If no system is given use the current system.
   * If we're searching a catalog and no stability is given, use "stable".
   */
  argparse::Argument & addAttrPathArgs( argparse::ArgumentParser & parser );

  /**
   * Sets fallback `attrPath` to a package set.
   * If `attrPath` is empty use, `packages.<SYTEM>`.
   * If `attrPath` is one element then add "current system" as `<SYSTEM>`.
   * If `attrPath` is a catalog with no stability use `stable`.
   */
  void fixupAttrPath();

  inline void postProcessArgs() override { this->fixupAttrPath(); }

};  /* End struct `AttrPathMixin' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::command' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
