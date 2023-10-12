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

#include <string>
#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
#include "flox/resolver/params.hh"
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
struct ManifestDescriptorRaw {

  public:

    /** Match `name`, `pname`, or `pkgAttrName` */
    std::optional<std::string> name;

    /** Match `version` or `semver` if a modifier is present. */
    std::optional<std::string> version;

    /** Match a catalog stability. */
    std::optional<std::string> stability;

    /** @brief A dot separated attribut path, or list representation. */
    using Path = std::variant<std::string, flox::AttrPath>;
    /** Match a relative path. */
    std::optional<Path> path;

    /**
     * @brief A dot separated attribut path, or list representation.
     *        May contain `null` members to represent _globs_.
     */
    using AbsPath = std::variant<std::string, AttrPathGlob>;
    /** Match an absolute path, allowing globs for `system`. */
    std::optional<AbsPath> absPath;

    /** Only resolve for a given set of systems. */
    std::optional<std::vector<std::string>> systems;

    /** Whether resoution is allowed to fail without producing errors. */
    std::optional<bool> optional;

    /** Named _group_ that the package is a member of. */
    std::optional<std::string> packageGroup;

    /** Force resolution is a given input or _flake reference_. */
      std::optional<std::variant<std::string, nix::fetchers::Attrs>>
    packageRepository;

    /** Relative path to a `nix` expression file to be evaluated. */
    std::optional<std::string> input;


};  /* End struct `ManifestDescriptorRaw' */


/* -------------------------------------------------------------------------- */

/**
 * @fn void from_json( const nlohmann::json        & jfrom
 *                   ,       ManifestDescriptorRaw & desc
 *                   )
 * @brief Convert a JSON object to an @a flox::InputPreferences.
 *
 * @fn void to_json( nlohmann::json & jto, const ManifestDescriptorRaw & desc )
 * @brief Convert an @a flox::resolver::ManifestDescriptorRaw to a JSON object.
 */
/* Generate to_json/from_json functions. */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT( ManifestDescriptorRaw
                                               , name
                                               , version
                                               , stability
                                               , path
                                               , absPath
                                               , systems
                                               , optional
                                               , packageGroup
                                               , packageRepository
                                               , input
                                               )


/* -------------------------------------------------------------------------- */

/**
 * @brief Extend and remap fields from @a flox::resolver::PkgDescriptorRaw to
 *        @a flox::pkgdb::PkgQueryArgs
 */
struct ManifestDescriptor {

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

    /** Match a catalog stability. */
    std::optional<std::string> stability;

    /** Match a relative attribute path. */
    std::optional<flox::AttrPath> path;

    /** Force resolution is a given input, _flake reference_, or file. */
    std::optional<std::variant<nix::FlakeRef, std::string>> input;


    ManifestDescriptor() = default;

    explicit ManifestDescriptor( const ManifestDescriptorRaw & raw );


    /** @brief Reset to default state. */
    void clear();

    /**
     * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
     *        lookup packages.
     *
     * NOTE: This DOES NOT clear @a pqa before filling it.
     * This is intended to be used after filling @a pqa with global preferences.
     * @param pqa A set of query args to _fill_ with preferences.
     * @return A reference to the modified query args.
     */
    pkgdb::PkgQueryArgs & fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const;


};  /* End struct `ManifestDescriptor' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
