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

#include <nlohmann/json.hpp>


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

/* Generate `to_json' and `from_json' for enum. */
NLOHMANN_JSON_SERIALIZE_ENUM( subtree_type, {
  { ST_NONE,     nullptr          }
, { ST_LEGACY,   "legacyPackages" }
, { ST_PACKAGES, "packages"       }
, { ST_CATALOG,  "catalog"        }
} )


/** @brief A strongly typed wrapper over an attribute path _subtree_ name. */
struct Subtree {

  subtree_type subtree = ST_NONE;

  constexpr Subtree() = default;

  // NOLINTNEXTLINE
  constexpr Subtree( subtree_type subtree ) : subtree( subtree ) {}

  constexpr explicit Subtree( std::string_view str ) noexcept
    : subtree( ( str == "legacyPackages" ) ? ST_LEGACY   :
               ( str == "packages"       ) ? ST_PACKAGES :
               ( str == "catalog"        ) ? ST_CATALOG  : ST_NONE
             )
  {}

    [[nodiscard]]
    static Subtree
  parseSubtree( std::string_view str )
  {
    return Subtree {
      ( str == "legacyPackages" ) ? ST_LEGACY   :
      ( str == "packages"       ) ? ST_PACKAGES :
      ( str == "catalog"        ) ? ST_CATALOG  :
      throw std::invalid_argument(
        "Invalid subtree '" + std::string( str ) + "'"
      )
    };
  }

    [[nodiscard]] friend constexpr
    std::string_view
  to_string( const Subtree & subtree )
  {
    switch ( subtree.subtree )
      {
        case ST_LEGACY:   return "legacyPackages";
        case ST_PACKAGES: return "packages";
        case ST_CATALOG:  return "catalog";
        default:          return "ST_NONE";
      }
  }

  constexpr explicit operator std::string_view() const
  {
    return to_string( * this );
  }

  // NOLINTNEXTLINE
  constexpr operator subtree_type() const { return this->subtree; }

  constexpr bool operator==( const Subtree & other ) const = default;
  constexpr bool operator!=( const Subtree & other ) const = default;

    constexpr bool
  operator==( const subtree_type & other ) const
  {
    return this->subtree == other;
  }

    constexpr bool
  operator!=( const subtree_type & other ) const
  {
    return this->subtree != other;
  }

};  /* End struct `Subtree' */


/* -------------------------------------------------------------------------- */

  inline void
from_json( const nlohmann::json & jfrom, Subtree & subtree )
{
  jfrom.get_to( subtree.subtree );
}

  inline void
to_json( nlohmann::json & jto, const Subtree & subtree )
{
  jto = to_string( subtree );
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
