/* ========================================================================== *
 *
 * @file flox/resolver/descriptor.hh
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

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Extend and remap fields from @a flox::resolver::PkgDescriptorRaw to
 *        those found in a `flox` _manifest_.
 *
 * This _raw_ struct is defined to generate parsers.
 * The _real_ form is @a flox::resolver::ManifestDescriptor.
 */
struct ManifestDescriptorRaw
{

public:

  /**
   * Match `name`, `pname`, or `pkgAttrName`.
   * Maps to `flox::pkgdb::PkgQueryArgs::pnameOrPkgAttrName`.
   */
  std::optional<std::string> name;

  /**
   * Match `version` or `semver` if a modifier is present.
   *
   * Strings beginning with an `=` will filter by exact match on `version`.
   * Any string which may be interpreted as a semantic version range will
   * filter on the `semver` field.
   * All other strings will filter by exact match on `version`.
   */
  std::optional<std::string> version;

  /** @brief A dot separated attribut path, or list representation. */
  using Path = std::variant<std::string, flox::AttrPath>;
  /** Match a relative path. */
  std::optional<Path> path;

  /**
   * @brief A dot separated attribut path, or list representation.
   *        May contain `null` members to represent _globs_.
   *
   * NOTE: `AttrPathGlob` is a `std::vector<std::optional<std::string>>`
   *       which represnts an absolute attribute path which may have
           `std::nullopt` as its second element to avoid indicating a
           particular system.
   */
  using AbsPath = std::variant<std::string, AttrPathGlob>;
  /** Match an absolute path, allowing globs for `system`. */
  std::optional<AbsPath> absPath;

  /** Only resolve for a given set of systems. */
  std::optional<std::vector<std::string>> systems;

  /** Whether resoution is allowed to fail without producing errors. */
  std::optional<bool> optional;

  // TODO: Not implemented.
  /** Named _group_ that the package is a member of. */
  std::optional<std::string> packageGroup;

  // TODO: Not implemented.
  /** Force resolution is the named input or _flake reference_. */
  std::optional<std::variant<std::string, nix::fetchers::Attrs>>
    packageRepository;


  /**
   * Rank a package's priority for handling conflicting files.
   * The default value is `5` ( set in @a flox::resolver::ManifestDescriptor ).
   *
   * Packages with higher @a priority values will take precendence over those
   * with lower @a priority values.
   */
  std::optional<unsigned> priority;


}; /* End struct `ManifestDescriptorRaw' */


/* -------------------------------------------------------------------------- */

// TODO: support `packageRepository' field
/**
 * @brief Convert a JSON object to an @a flox::ManifestDescriptorRaw. */
void
from_json( const nlohmann::json & jfrom, ManifestDescriptorRaw & descriptor );
/**
 * @brief Convert an @a flox::resolver::ManifestDescriptorRaw to a
 *              JSON object.
 */
void
to_json( nlohmann::json & jto, const ManifestDescriptorRaw & descriptor );

/**
 * @class flox::pkgdb::ParseManifestDescriptorRawException
 * @brief An exception thrown when parsing @a flox::resolver::ManifestDescriptorRaw
 *        from JSON.
 */
FLOX_DEFINE_EXCEPTION( ParseManifestDescriptorRawException,
                       EC_PARSE_MANIFEST_DESCRIPTOR_RAW,
                       "error parsing manifest descriptor" )

/* -------------------------------------------------------------------------- */

/**
 * @brief Extend and remap fields from @a flox::resolver::PkgDescriptorRaw to
 *        @a flox::pkgdb::PkgQueryArgs
 */
struct ManifestDescriptor
{

public:

  /** Match `name`, `pname`, or `pkgAttrName` */
  std::optional<std::string> name;

  /** Whether resoution is allowed to fail without producing errors. */
  bool optional = false;

  /** Named _group_ that the package is a member of. */
  std::optional<std::string> group;

  /** Match `version`. */
  std::optional<std::string> version;

  /** Match a semantic version range. */
  std::optional<std::string> semver;

  /** Match a subtree. */
  std::optional<Subtree> subtree;

  /** Only resolve for a given set of systems. */
  std::optional<std::vector<std::string>> systems;

  /** Match a relative attribute path. */
  std::optional<flox::AttrPath> path;

  /** Force resolution is a given input, _flake reference_. */
  std::optional<nix::FlakeRef> input;

  /**
   * Rank a package's priority for handling conflicting files.
   * The default value is `5` ( set in @a flox::resolver::ManifestDescriptor ).
   *
   * Packages with higher @a priority values will take precendence over those
   * with lower @a priority values.
   */
 unsigned priority = 5;


  ManifestDescriptor() = default;

  explicit ManifestDescriptor( const ManifestDescriptorRaw & raw );


  /** @brief Reset to default state. */
  void
  clear();

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


}; /* End struct `ManifestDescriptor' */


/* -------------------------------------------------------------------------- */

/**
 * @class flox::resolver::InvalidManifestDescriptorException
 * @brief An exception thrown when a package descriptor in a manifest
 *        is invalid.
 */
FLOX_DEFINE_EXCEPTION( InvalidManifestDescriptorException,
                       EC_INVALID_MANIFEST_DESCRIPTOR,
                       "invalid manifest descriptor" )


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
