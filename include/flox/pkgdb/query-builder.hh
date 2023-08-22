/* ========================================================================== *
 *
 * @file flox/pkgdb/query-builder.hh
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <functional>

#include <nix/shared.hh>

#include "flox/core/types.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb {

/* -------------------------------------------------------------------------- */

struct PkgQueryArgs {

  /** Match partial name/description */
  std::optional<std::string> match;
  std::optional<std::string> name;
  std::optional<std::string> pname;
  std::optional<std::string> version;
  std::optional<std::string> semver;

  std::optional<std::vector<std::string>> licenses;

  bool allowBroken       = false;
  bool allowUnfree       = true;
  bool preferPreReleases = false;

  std::optional<std::vector<subtree_type>> subtrees;

  std::vector<std::string> systems = { nix::settings.thisSystem.get() };

  std::optional<std::vector<std::string>> stabilities;

  // TODO
  ///**
  // * Sanity check parameters.
  // * Make sure `systems` are valid systems.
  // * Make sure `stabilities` are valid stabilities.
  // * Make sure `name` is not set when `pname`, `version`, or `semver` are set.
  // * Make sure `version` is not set when `semver` is set.
  // * @return `true` iff the above conditions are met.
  // */
  //bool validate();

};  /* End struct `PkgQueryArgs' */


/* -------------------------------------------------------------------------- */

using SQLBinds = std::unordered_map<std::string, std::string>;

std::pair<std::string, SQLBinds> buildPkgQuery( const PkgQueryArgs & params );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
