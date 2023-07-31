/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <iterator>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/exceptions.hh"
#include "flox/util.hh"
#include <queue>
#include <any>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

using FloxFlakeRef = nix::FlakeRef;
using input_pair   =
  std::pair<std::string, std::shared_ptr<nix::flake::LockedFlake>>;

using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using CursorPos   = std::pair<Cursor, std::vector<nix::Symbol>>;
using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;


/* -------------------------------------------------------------------------- */

/**
 * A queue of cursors used to stash sub-attrsets that need to be searched
 * recursively in various iterators.
 */
using todo_queue = std::queue<Cursor, std::list<Cursor>>;


/* -------------------------------------------------------------------------- */

class Descriptor;
class Package;
class DrvDb;


/* -------------------------------------------------------------------------- */

typedef enum {
  ST_NONE     = 0
, ST_PACKAGES = 1
, ST_LEGACY   = 2
, ST_CATALOG  = 3
} subtree_type;

NLOHMANN_JSON_SERIALIZE_ENUM( subtree_type, {
  { ST_NONE,     nullptr          }
, { ST_PACKAGES, "packages"       }
, { ST_LEGACY,   "legacyPackages" }
, { ST_CATALOG,  "catalog"        }
} )

subtree_type     parseSubtreeType( std::string_view subtree );
std::string_view subtreeTypeToString( const subtree_type & st );


/* -------------------------------------------------------------------------- */

typedef enum {
  SY_NONE     = 0
, SY_STABLE   = 1
, SY_STAGING  = 2
, SY_UNSTABLE = 3
} stability_type;

NLOHMANN_JSON_SERIALIZE_ENUM( stability_type, {
  { SY_NONE,     nullptr    }
, { SY_STABLE,   "stable"   }
, { SY_STAGING,  "staging"  }
, { SY_UNSTABLE, "unstable" }
} )


/* -------------------------------------------------------------------------- */

typedef enum {
  DBPS_NONE       = 0 /* Indicates that a DB is completely fresh. */
, DBPS_PARTIAL    = 1 /* Indicates some partially populated state. */
, DBPS_PATHS_DONE = 2 /* Indicates that we know all derivation paths. */
, DBPS_INFO_DONE  = 3 /* Indicates that we have collected info metadata. */
, DBPS_EMPTY      = 4 /* Indicates that a prefix has no values. */
, DBPS_MISSING    = 5 /* Indicates that a DB is completely fresh. */
, DBPS_FORCE      = 6 /* This should always have highest value. */
}  progress_status;


/* -------------------------------------------------------------------------- */

using attr_part  = std::variant<std::nullptr_t, std::string>;
using attr_parts = std::vector<attr_part>;

struct AttrPathGlob {

  attr_parts path = {};

  static AttrPathGlob fromStrings( const std::vector<std::string>      & pp );
  static AttrPathGlob fromStrings( const std::vector<std::string_view> & pp );
  static AttrPathGlob fromJSON(    const nlohmann::json                & pp );

  AttrPathGlob()                        = default;
  AttrPathGlob( const AttrPathGlob &  ) = default;
  AttrPathGlob(       AttrPathGlob && ) = default;
  AttrPathGlob( const attr_parts &  pp );
  AttrPathGlob(       attr_parts && pp );

  AttrPathGlob & operator=( const AttrPathGlob & ) = default;

  std::string    toString() const;
  nlohmann::json toJSON()   const;

  bool isAbsolute() const;
  bool hasGlob()    const;
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceRelative();
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceGlob();

  bool globEq(     const AttrPathGlob & other ) const;
  bool operator==( const AttrPathGlob & other ) const;

  size_t size() const { return this->path.size(); }

};

void from_json( const nlohmann::json & j,       AttrPathGlob & path );
void to_json(         nlohmann::json & j, const AttrPathGlob & path );


/* -------------------------------------------------------------------------- */

class Inputs {
  private:
    std::unordered_map<std::string, FloxFlakeRef> inputs;

    void init( const nlohmann::json & j );

  public:

    Inputs() = default;
    Inputs( const nlohmann::json & j ) { this->init( j ); }

    bool           has( std::string_view id ) const;
    FloxFlakeRef   get( std::string_view id ) const;
    nlohmann::json toJSON()                   const;

    std::list<std::string_view> getInputNames() const;
};


void from_json( const nlohmann::json & j,       Inputs & i );
void to_json(         nlohmann::json & j, const Inputs & i );


/* -------------------------------------------------------------------------- */

namespace predicates { struct PkgPred; };

struct Preferences {
  std::vector<std::string> inputs;

  std::unordered_map<std::string, std::vector<std::string>> stabilities;
  std::unordered_map<std::string, std::vector<std::string>> prefixes;

  bool semverPreferPreReleases = false;
  bool allowUnfree             = true;
  bool allowBroken             = false;

  std::optional<std::unordered_set<std::string>> allowedLicenses;

  Preferences() = default;
  Preferences( const nlohmann::json & j );

  nlohmann::json toJSON() const;

  flox::resolve::predicates::PkgPred pred_V2() const;

  int compareInputs(
        const std::string_view idA, const FloxFlakeRef & a
      , const std::string_view idB, const FloxFlakeRef & b
      ) const;

    inline int
  compareInputs( const input_pair & a, const input_pair & b ) const
  {
    return this->compareInputs( a.first
                              , a.second->flake.lockedRef
                              , b.first
                              , b.second->flake.lockedRef
                              );
  }

    std::function<bool( const input_pair &, const input_pair & )>
  inputLessThan() const
  {
    return [&]( const input_pair & a, const input_pair & b )
    {
      return this->compareInputs( a, b ) < 0;
    };
  }
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- */

  template<>
struct std::hash<flox::resolve::AttrPathGlob>
{
    std::size_t
  operator()( const flox::resolve::AttrPathGlob & k ) const noexcept
  {
    if ( k.path.size() < 1 ) { return 0; }
    std::size_t h1 = std::hash<std::string>{}(
      std::get<std::string>( k.path[0] )
    );
    for ( size_t i = 1; i < k.path.size(); ++i )
      {
        if ( std::holds_alternative<std::string>( k.path[1] ) )
          {
            std::string p = std::get<std::string>( k.path[i] );
            if ( p != "{{system}}" )
              {
                std::size_t h2 = std::hash<std::string>{}( p );
                h1 = ( h1 >> 1 ) ^ ( h2 << 1 );
              }
          }
      }
    return h1;
  }
};


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
