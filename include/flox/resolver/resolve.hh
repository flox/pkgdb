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
  AttrPathGlob   path;   /**< Attribute path to the package. */
  nlohmann::json info;   /**< Package information. */

  /**@brief Reset to default/empty state. */
  void clear();

};  /* End struct `Resolved' */

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
std::vector<Resolved> resolve_V0(       ResolverState & state
                                , const Descriptor    & descriptor
                                ,       bool            one        = false
                                );


constexpr auto resolve = resolve_V0;


/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a package descriptor to its best candidate ( if any ).
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @return The best resolved installable or `std:nullopt` if resolution failed.
 */
std::optional<Resolved> resolveOne_V0(       ResolverState & state
                                     , const Descriptor    & descriptor
                                     );


constexpr auto resolveOne = resolveOne_V0;


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
