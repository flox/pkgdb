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

#include <string>
#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
#include "flox/pkgdb/params.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * An attribute path which may contain `null` members to represent _globs_.
 * Globs may only appear as the second element representing `system`.
 */
using AttrPathGlob = std::vector<std::optional<std::string>>;


/* -------------------------------------------------------------------------- */

/**
 * @brief A set of query parameters describing _requirements_ for a package.
 *
 * In its _raw_ form, we do not expect that "global" filters have been pushed
 * down into the descriptor, and do not attempt to distinguish from relative or
 * absolute attribute paths in the @a path field.
 */
struct PkgDescriptorRaw : pkgdb::PkgDescriptorBase {

  /* From `pkgdb::PkgDescriptorBase`:
   *   std::optional<std::string> name;
   *   std::optional<std::string> pname;
   *   std::optional<std::string> version;
   *   std::optional<std::string> semver;
   */

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

  /** Restricts resolution to a given stability. */
  std::optional<std::string> stability;

  /**
   * Whether pre-releases should be preferred over releases.
   *
   * Takes priority over `semver.preferPreReleases` "global" setting.
   */
  std::optional<bool> preferPreReleases = false;


  /** @brief Reset to default state. */
  void clear();

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages.
   *
   * This DOES NOT clear @a pqa before filling it.
   * This is intended to be used after filling @a pqa with global preferences.
   */
  pkgdb::PkgQueryArgs & fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;


};  /* End struct `PkgDescriptorRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::PkgDescriptorRaw. */
void from_json( const nlohmann::json & jfrom, PkgDescriptorRaw & desc );

/** @brief Convert a @a flox::resolver::PkgDescriptorRaw to a a JSON object. */
void to_json( nlohmann::json & jto, const PkgDescriptorRaw & desc );


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
struct ResolveOneParams : public pkgdb::QueryPreferences {

  /* QueryPreferences provides:
   *   std::vector<std::string> systems;
   *   struct Allows {
   *     bool unfree = true;
   *     bool broken = false;
   *     std::optional<std::vector<std::string>> licenses;
   *   } allow;
   *   struct Semver {
   *     preferPreReleases = false;
   *   } semver;
   */

  /** Settings and fetcher information associated with inputs. */
  RegistryRaw registry;

  /**
   * @brief A single package descriptor in _raw_ form.
   *
   * This requires additional post-processing, such as "pushing down" global
   * settings, before it can be used to perform resolution.
   */
  PkgDescriptorRaw query;


  /** @brief Reset to default/empty state. */
  void clear();

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages in a particular input.
   * @param input The input name to be searched.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return `true` if @a pqa was modified, indicating that the input should be
   *         searched, `false` otherwise.
   */
  bool fillPkgQueryArgs( const std::string         & input
                       ,       pkgdb::PkgQueryArgs & pqa
                       ) const;

};  /* End struct `ResolveOneParams' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ResolveOneParams. */
void from_json( const nlohmann::json & jfrom, ResolveOneParams & params );

/** @brief Convert a @a flox::resolver::ResolveOneParams to a a JSON object. */
void to_json( nlohmann::json & jto, const ResolveOneParams & params );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
