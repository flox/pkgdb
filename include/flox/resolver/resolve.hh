/* ========================================================================== *
 *
 * @file flox/resolver/resolve.hh
 *
 * @brief Resolve package descriptors in flakes.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

#include "flox/resolver/state.hh"


/* -------------------------------------------------------------------------- */

/** @brief Resolve package descriptors in flakes. */
namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/** @brief A _resolved_ installable resulting from resolution. */
struct Resolved {

  /** @brief A registry input. */
  struct Input {
    std::string    name;    /**< Registry input name/id. */
    nlohmann::json locked;  /**< Locked flake ref attributes. */
  };  /* End struct `Resolved::Input' */

  Input          input;  /**< Registry input. */
  AttrPath       path;   /**< Attribute path to the package. */
  nlohmann::json info;   /**< Package information. */

  /**@brief Reset to default/empty state. */
  void clear();

};  /* End struct `Resolved' */

/**
 * @fn void from_json( const nlohmann::json & j, Resolved::Input & pdb )
 * @brief Convert a JSON object to a @a flox::resolver::Resolved::Input.
 *
 * @fn void to_json( nlohmann::json & j, const Resolved::Input & pdb )
 * @brief Convert a @a flox::resolver::Resolved::Input to a JSON object.
 *
 * @fn void from_json( const nlohmann::json & j, Resolved & pdb )
 * @brief Convert a JSON object to a @a flox::resolver::Resolved.
 *
 * @fn void to_json( nlohmann::json & j, const Resolved & pdb )
 * @brief Convert a @a flox::resolver::Resolved to a JSON object.
 */
/* Generate `to_json' and `from_json' functions. */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( Resolved::Input, name, locked )
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( Resolved, input, path, info )


/* -------------------------------------------------------------------------- */

using Descriptor = PkgDescriptorRaw;


/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a package descriptor.
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @param one If `true`, return only the first result.
 * @return A list of resolved packages.
 */
std::vector<Resolved> resolve_v0(       ResolverState & state
                                , const Descriptor    & descriptor
                                ,       bool            one        = false
                                );


constexpr auto resolve = resolve_v0;


/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a package descriptor to its best candidate ( if any ).
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @return The best resolved installable or `std:nullopt` if resolution failed.
 */
  std::optional<Resolved>
resolveOne_v0(       ResolverState & state
             , const Descriptor    & descriptor
             )
{
  auto resolved = resolve( state, descriptor, true );
  if ( resolved.empty() ) { return std::nullopt; }
  return resolved[0];
}


constexpr auto resolveOne = resolveOne_v0;


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
