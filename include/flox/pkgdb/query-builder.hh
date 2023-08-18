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

#include <nix/shared.hh>
#include <sql-builder/sql.hh>
#include <sqlite3pp.hh>

#include "flox/core/types.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb {

/* -------------------------------------------------------------------------- */

using PkgQuery = sql::SelectModel;

/* -------------------------------------------------------------------------- */

struct PkgQueryArgs {
  /** Match partial name/description */
  std::optional<std::string>    match;
  std::optional<flox::AttrPath> pathPrefix;
  std::optional<subtree_type>   subtree;
  std::optional<std::string>    stability;
  std::optional<std::string>    system;

  std::optional<std::string>            name;
  std::optional<std::string>            pname;
  std::optional<std::string>            version;
  std::optional<std::string>            semver;
  std::optional<std::list<std::string>> licenses;

  bool allowBroken       = false;
  bool allowUnfree       = true;
  bool preferPreReleases = false;

  std::optional<std::list<subtree_type>> subtrees;

  std::list<std::string>  systems     = { nix::settings.thisSystem.get() };
  std::list<std::string>  stabilities = flox::defaultCatalogStabilities;
};  /* End struct `PkgQueryArgs' */


PkgQuery buildPkgQuery( PkgQueryArgs && params );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
