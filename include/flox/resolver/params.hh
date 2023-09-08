/* ========================================================================== *
 *
 * @file flox/resolver/params.hh
 *
 * @brief A set of user inputs used to set input preferences and query
 * parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
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
   * @brief An absolute or relative attribut path to a package.
   *
   * May contain `std::nullopt` as its second member to indicate that any system
   * is acceptable.
   */
  std::optional<AttrPathGlob> path;

  /**
   * @brief Restricts resolution to a given subtree.
   *
   * This field must not conflict with the @a path field.
   */
  std::optional<std::string> subtree;

  /** Restricts resolution to a given stability. */
  std::optional<std::string> stability;

  /**
   * @brief Whether pre-releases should be preferred over releases.
   *
   * Takes priority over `semver.preferPreReleases` "global" setting.
   */
  std::optional<bool> preferPreReleases = false;


  /** Reset to default state. */
    inline void
  clear()
  {
    this->pkgdb::PkgDescriptorBase::clear();
    this->input             = std::nullopt;
    this->path              = std::nullopt;
    this->subtree           = std::nullopt;
    this->stability         = std::nullopt;
    this->preferPreReleases = std::nullopt;
  }

};  /* End struct `PkgDescriptorRaw' */


/* -------------------------------------------------------------------------- */

void from_json( const nlohmann::json & jfrom,       PkgDescriptorRaw & desc );
void to_json(         nlohmann::json & jto,   const PkgDescriptorRaw & desc );


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
struct ResolveOneParams {

  /** Settings and fetcher information associated with inputs. */
  RegistryRaw registry;

  /**
   * Ordered list of systems to be searched.
   * Results will be grouped by system in the order they appear here.
   */
  std::vector<std::string> systems;


  /** Allow/disallow packages with certain metadata. */
  struct Allows {

    /** Whether to include packages which are explicitly marked `unfree`. */
    bool unfree = true;

    /** Whether to include packages which are explicitly marked `broken`. */
    bool broken = false;

    /** Filter results to those explicitly marked with the given licenses. */
    std::optional<std::vector<std::string>> licenses;

  } allow;


  /** Settings associated with semantic version processing. */
  struct Semver {

    /** Whether pre-release versions should be ordered before releases. */
    bool preferPreReleases = false;

  } semver;


  /**
   * @brief A single package descriptor in _raw_ form.
   *
   * This requires additional post-processing, such as "pushing down" global
   * settings, before it can be used to perform resolution.
   */
  PkgDescriptorRaw query;


  /** Reset preferences to default/empty state. */
  void clear();

};  /* End struct `ResolveOneParams' */


/* -------------------------------------------------------------------------- */

void from_json( const nlohmann::json & jfrom,       ResolveOneParams & params );
void to_json(         nlohmann::json & jto,   const ResolveOneParams & params );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
