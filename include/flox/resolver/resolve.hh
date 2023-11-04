/* ========================================================================== *
 *
 * @file flox/resolver/resolve.hh
 *
 * @brief Resolve package descriptors in flakes.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/registry.hh"
#include "flox/resolver/state.hh"


/* -------------------------------------------------------------------------- */

/** @brief Resolve package descriptors in flakes. */
namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/** @brief A _resolved_ installable resulting from resolution. */
struct Resolved
{

  /** @brief A registry input. */
  struct Input
  {
  private:

    /**
     * @brief Reduce @a locked fields to those needed to fetch the flake
     *        in pure evaluation mode.
     */
    void
    limitLocked();


  public:

    std::optional<std::string> name;   /**< Registry input name/id. */
    nlohmann::json             locked; /**< Locked flake ref attributes. */

    Input()                = default;
    Input( const Input & ) = default;
    Input( Input && )      = default;

    Input( std::string name, nlohmann::json locked )
      : name( std::move( name ) ), locked( std::move( locked ) )
    {
      this->limitLocked();
    }

    Input( std::string name, const nix::FlakeRef & locked )
      : name( std::move( name ) ), locked( locked )
    {
      this->limitLocked();
    }

    explicit Input( nlohmann::json locked ) : locked( std::move( locked ) )
    {
      this->limitLocked();
    }

    explicit Input( const nix::FlakeRef & locked ) : locked( locked )
    {
      this->limitLocked();
    }

    ~Input() = default;
    Input &
    operator=( const Input & )
      = default;
    Input &
    operator=( Input && )
      = default;

  }; /* End struct `Resolved::Input' */

  Input          input; /**< Registry input. */
  AttrPath       path;  /**< Attribute path to the package. */
  nlohmann::json info;  /**< Package information. */

  /**@brief Reset to default/empty state. */
  void
  clear();


}; /* End struct `Resolved' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::Resolved::Input. */
void
from_json( const nlohmann::json & jfrom, Resolved::Input & input );

/** @brief Convert a @a flox::resolver::Resolved::Input to a JSON object. */
void
to_json( nlohmann::json & jto, const Resolved::Input & input );


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::Resolved. */
void
from_json( const nlohmann::json & jfrom, Resolved & resolved );

/** @brief Convert a @a flox::resolver::Resolved to a JSON object. */
void
to_json( nlohmann::json & jto, const Resolved & resolved );


/* -------------------------------------------------------------------------- */

/**
 * @class flox::pkgdb::ParseResolvedException
 * @brief An exception thrown when parsing @a flox::resolver::Resolved
 *        from JSON.
 *
 * @{
 */
FLOX_DEFINE_EXCEPTION( ParseResolvedException,
                       EC_PARSE_RESOLVED,
                       "error parsing resolved installable" )
/** @} */


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
[[nodiscard]] std::vector<Resolved>
resolve_v0( ResolverState &    state,
            const Descriptor & descriptor,
            bool               one = false );


/**
 * @brief Resolve a package descriptor.
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @param one If `true`, return only the first result.
 * @return A list of resolved packages.
 */
#define resolve( ... ) resolve_v0( __VA_ARGS__ )


/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a package descriptor to its best candidate ( if any ).
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @return The best resolved installable or `std:nullopt` if resolution failed.
 */
[[nodiscard]] inline std::optional<Resolved>
resolveOne_v0( ResolverState & state, const Descriptor & descriptor )
{
  auto resolved = resolve( state, descriptor, true );
  if ( resolved.empty() ) { return std::nullopt; }
  return resolved[0];
}


/**
 * @brief Resolve a package descriptor to its best candidate ( if any ).
 * @param state The resolver state.
 * @param descriptor The package descriptor.
 * @return The best resolved installable or `std:nullopt` if resolution failed.
 */
#define resolveOne( ... ) resolveOne_v0( __VA_ARGS__ )


/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a package descriptor in a given flake.
 * @param preferences Settings controlling resolution.
 * @param flake The flake to resolve in.
 * @param descriptor The package descriptor.
 * @param one If `true`, return only the first result.
 */
[[nodiscard]] inline std::vector<Resolved>
resolveInFlake_v0( const pkgdb::QueryPreferences & preferences,
                   const RegistryInput &           flake,
                   const Descriptor &              descriptor,
                   bool                            one = false )
{
  RegistryRaw registry;
  registry.inputs.emplace( flake.getFlakeRef()->to_string(), flake );
  ResolverState state( registry, preferences );
  return resolve_v0( state, descriptor, one );
}


/**
 * @brief Resolve a package descriptor in a given flake.
 * @param preferences Settings controlling resolution.
 * @param flake The flake to resolve in.
 * @param descriptor The package descriptor.
 * @param one If `true`, return only the first result.
 */
[[nodiscard]] inline std::vector<Resolved>
resolveInFlake_v0( const pkgdb::QueryPreferences & preferences,
                   const nix::FlakeRef &           flake,
                   const Descriptor &              descriptor,
                   bool                            one = false )
{
  return resolveInFlake_v0( preferences,
                            RegistryInput( flake ),
                            descriptor,
                            one );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
