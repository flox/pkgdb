/* ========================================================================== *
 *
 * @file flox/registry.hh
 *
 * @brief A set of user inputs used to set input preferences during search
 *        and resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <vector>
#include <ranges>
#include <map>

#include <nlohmann/json.hpp>
#include <nix/flake/flakeref.hh>
#include <nix/fetchers.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** Preferences associated with a registry input. */
struct InputPreferences {

  /**
   * Ordered list of subtrees to be searched.
   * Results will be grouped by subtree in the order they appear here.
   */
  std::optional<std::vector<subtree_type>> subtrees;

  /**
   * Ordered list of stabilities to be searched.
   * Catalog results will be grouped by stability in the order they
   * appear here.
   */
  std::optional<std::vector<std::string>> stabilities;

};  /* End struct `InputPreferences' */

void from_json( const nlohmann::json & jfrom,       InputPreferences & prefs );
void to_json(         nlohmann::json & jfrom, const InputPreferences & prefs );


/* -------------------------------------------------------------------------- */

/** Preferences associated with a named registry input. */
struct RegistryInput : public InputPreferences {
  std::shared_ptr<nix::FlakeRef> from;
};  /* End struct `RegistryInput' */

void from_json( const nlohmann::json & jfrom,       RegistryInput & rip );
void to_json(         nlohmann::json & jfrom, const RegistryInput & rip );

/* -------------------------------------------------------------------------- */

/**
 * @brief A set of user inputs used to set input preferences during search
 *        and resolution.
 *
 * @example
 * ```
 * {
 *   "inputs": {
 *     "nixpkgs": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "NixOS"
 *       , "repo": "nixpkgs"
 *       }
 *     , "subtrees": ["legacyPackages"]
 *     }
 *   , "floco": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "aakropotkin"
 *       , "repo": "floco"
 *       }
 *     , "subtrees": ["packages"]
 *     }
 *   , "floxpkgs": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "flox"
 *       , "repo": "floxpkgs"
 *       }
 *     , "subtrees": ["catalog"]
 *     , "stabilities": ["stable"]
 *     }
 *   }
 * , "defaults": {
 *     "subtrees": null
 *   , "stabilities": ["stable"]
 *   }
 * , "priority": ["nixpkgs", "floco", "floxpkgs"]
 * }
 * ```
 */
struct Registry {

  /** Settings and fetcher information associated with named inputs. */
  std::map<std::string, RegistryInput> inputs;

  /** Default/fallback settings for inputs. */
  InputPreferences defaults;

  /**
   * Priority order used to process inputs.
   * Inputs which do not appear in this list are handled in lexicographical
   * order after any explicitly named inputs.
   */
  std::vector<std::string> priority;

  /** @return A `std::ranges::ref_view` of input names in order of priority. */
  auto getOrder() const;

  /** Reset to default state. */
    inline void
  clear()
  {
    this->inputs.clear();
    this->priority.clear();
  }

};  /* End struct `Registry' */


void from_json( const nlohmann::json & jfrom,       Registry & reg );
void to_json(         nlohmann::json & jfrom, const Registry & reg );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
