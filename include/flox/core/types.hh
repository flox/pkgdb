/* ========================================================================== *
 *
 * @file flox/core/types.hh
 *
 * @brief Miscellaneous typedefs and aliases
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>

#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include <nix/ref.hh>


/* -------------------------------------------------------------------------- */

/** Interfaces for use by `flox`. */
namespace flox {

/* -------------------------------------------------------------------------- */

/** A list of key names addressing a location in a nested JSON-like object. */
using AttrPath = std::vector<std::string>;

/** A `std::shared_ptr<nix::eval_cache::AttrCursor>` which may be `nullptr`. */
using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;

/** A non-`nullptr` `std::shared_ptr<nix::eval_cache::AttrCursor>`. */
using Cursor = nix::ref<nix::eval_cache::AttrCursor>;


/* -------------------------------------------------------------------------- */

/** A _top level_ key in a `nix` flake */
enum subtree_type {
  ST_NONE     = 0
, ST_LEGACY   = 1
, ST_PACKAGES = 2
, ST_CATALOG  = 3
};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
