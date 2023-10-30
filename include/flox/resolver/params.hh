/* ========================================================================== *
 *
 * @file flox/resolver/params.hh
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "flox/pkgdb/params.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief A set of query parameters describing _requirements_ for a package.
 *
 * In its _raw_ form, we DO NOT expect that "global" filters have been pushed
 * down into the descriptor, and do not attempt to distinguish from relative or
 * absolute attribute paths in the @a path field.
 */
struct PkgDescriptorRaw : public pkgdb::PkgDescriptorBase
{

  /* From `pkgdb::PkgDescriptorBase':
   *   std::optional<std::string> name;    //< Filter results by exact `name`.
   *   std::optional<std::string> pname;   //< Filter results by exact `pname`.
   *   std::optional<std::string> version; //< Filter results by exact version.
   *   std::optional<std::string> semver;  //< Filter results by version range.

   */

  /**
   * Filter results by an exact match on either `pname` or `pkgAttrName`.
   * To match just `pname` see @a flox::pkgdb::PkgDescriptorBase.
   */
  std::optional<std::string> pnameOrPkgAttrName;

  /** Restricts resolution to the named registry input. */
  std::optional<std::string> input;

  /**
   * An absolute or relative attribut path to a package.
   *
   * May contain `std::nullopt` as its second member to indicate that any system
   * is acceptable.
   */
  std::optional<AttrPathGlob> path;

  /**
   * Restricts resolution to a given subtree.
   * Restricts resolution to a given subtree.
   *
   * This field must not conflict with the @a path field.
   */
  std::optional<std::string> subtree;

  /**
   * Whether pre-releases should be preferred over releases.
   *
   * Takes priority over `semver.preferPreReleases` "global" setting.
   */
  std::optional<bool> preferPreReleases = false;


  /** @brief Reset to default state. */
  void
  clear() override;

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages.
   *
   * NOTE: This DOES NOT clear @a pqa before filling it.
   * This is intended to be used after filling @a pqa with global preferences.
   * @param pqa A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs &
  fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;


}; /* End struct `PkgDescriptorRaw' */


/* -------------------------------------------------------------------------- */

/**
 * @class flox::resolver::InvalidPkgDescriptorException
 * @brief An exception thrown a PkgDescriptorRaw is invalid
 */
FLOX_DEFINE_EXCEPTION( InvalidPkgDescriptorException,
                       EC_INVALID_PKG_DESCRIPTOR,
                       "invalid package descriptor" )


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::PkgDescriptorRaw. */
void
from_json( const nlohmann::json & jfrom, PkgDescriptorRaw & desc );

/** @brief Convert a @a flox::resolver::PkgDescriptorRaw to a a JSON object. */
void
to_json( nlohmann::json & jto, const PkgDescriptorRaw & desc );


/* -------------------------------------------------------------------------- */

/**
 * @brief A set of resolution parameters for resolving a single descriptor.
 *
 * This is a trivially simple form of resolution which does not consider
 * _groups_ of descriptors or attempt to optimize with additional context.
 *
 * This is essentially a reorganized form of @a flox::pkgdb::PkgQueryArgs
 * that is suited for JSON input.
 */
using ResolveOneParams = pkgdb::QueryParams<PkgDescriptorRaw>;


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
