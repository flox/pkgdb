/* ========================================================================== *
 *
 * @file flox/package.hh
 *
 * @brief Abstract representation of a package.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include <nix/eval-cache.hh>
#include <nix/fetchers.hh>
#include <nix/flake/flake.hh>
#include <nix/names.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "versions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * @brief Abstract representation of a "package", analogous to a
 *        Nix `derivation'.
 *
 * This abstraction provides a common base for various backends that store,
 * evaluate, and communicate package definitions.
 */
class Package
{

public:

  /** @return attribute path where package is defined */
  virtual AttrPath
  getPathStrs() const
    = 0;

  /** @return the derivation `name` field. */
  virtual std::string
  getFullName() const
    = 0;

  /**
   * @return iff the field `pname` is defined then `pname`, otherwise the
   *         `name` field stripped of is _version_ part as recognized by
   *         `nix::DrvName` parsing rules.
   */
  virtual std::string
  getPname() const
    = 0;

  /**
   * @return iff the field `version` is defined then `version`, otherwise the
   *         `name` field stripped of is _pname_ part as recognized by
   *         `nix::DrvName` parsing rules.
   *         If `version` is undefined and `name` contains no version suffix,
   *         then `std::nullopt`.
   */
  virtual std::optional<std::string>
  getVersion() const = 0;

  /** @return The `meta.license.spdxId` field if defined,
   *          otherwise `std::nullopt` */
  virtual std::optional<std::string>
  getLicense() const = 0;

  /** @return The derivation `outputs` list. */
  virtual std::vector<std::string>
  getOutputs() const = 0;

  /**
   * @return The `meta.outputsToInstall` field if defined, otherwise the
   *         derivation `outputs` members to the left of and
   *         including `out`.
   */
  virtual std::vector<std::string>
  getOutputsToInstall() const = 0;

  /** @return The `meta.broken` field if defined, otherwise `std::nullopt`. */
  virtual std::optional<bool>
  isBroken() const = 0;

  /** @return The `meta.unfree` field if defined, otherwise `std::nullopt`. */
  virtual std::optional<bool>
  isUnfree() const = 0;

  /**
   * @return The `meta.description` field if defined,
   * otherwise `std::nullopt`.
   */
  virtual std::optional<std::string>
  getDescription() const = 0;

  /**
   * @return The flake `outputs` subtree the package resides in, being one of
   *         `legacyPackages` or `packages`.
   */
  virtual Subtree
  getSubtreeType() const
  {
    return Subtree( this->getPathStrs().front() );
  }

  /**
   * @return The parsed "package name" prefix of @a this package's
   *         `name` field.
   */
  virtual nix::DrvName
  getParsedDrvName() const
  {
    return nix::DrvName( this->getFullName() );
  }

  /**
   * @return `std::nullopt` iff @a this package does not use semantic
   *         versioning, otherwise a normalized semantic version number
   *         coerces from @a this package's `version` information.
   */
  virtual std::optional<std::string>
  getSemver() const
  {
    std::optional<std::string> version = this->getVersion();
    if ( ! version.has_value() ) { return std::nullopt; }
    return versions::coerceSemver( *version );
  }

  /**
   * @brief Create an installable URI string associated with this package
   *        using @a ref as its _input_ part.
   * @param ref Input flake reference associated with @a this package.
   *            This is used to construct the URI on the left side of `#`.
   * @return An installable URI string associated with this package using.
   */
  virtual std::string
  toURIString( const nix::FlakeRef & ref ) const
  {
    std::stringstream uri;
    uri << ref.to_string() << "#";
    AttrPath pathS = this->getPathStrs();
    for ( size_t i = 0; i < pathS.size(); ++i )
      {
        uri << '"' << pathS.at( i );
        if ( ( i + 1 ) < pathS.size() ) { uri << "."; }
      }
    return uri.str();
  }

  /**
   * @brief Serialize notable package metadata as a JSON object.
   *
   * This may only contains a subset of all available information.
   * @param withDescription Whether to include `description` strings.
   * @return A JSON object with notable package metadata.
   */
  virtual nlohmann::json
  getInfo( bool withDescription = false ) const
  {
    System system = this->getPathStrs().at( 1 );

    nlohmann::json jto = { { system,
                             { { "name", this->getFullName() },
                               { "pname", this->getPname() } } } };

    std::optional<std::string> oos = this->getVersion();

    if ( oos.has_value() ) { jto[system].emplace( "version", *oos ); }
    else { jto[system].emplace( "version", nlohmann::json() ); }

    oos = this->getSemver();
    if ( oos.has_value() ) { jto[system].emplace( "semver", *oos ); }
    else { jto[system].emplace( "semver", nlohmann::json() ); }

    jto[system].emplace( "outputs", this->getOutputs() );
    jto[system].emplace( "outputsToInstall", this->getOutputsToInstall() );

    oos = this->getLicense();
    if ( oos.has_value() ) { jto[system].emplace( "license", *oos ); }
    else { jto[system].emplace( "license", nlohmann::json() ); }

    std::optional<bool> obool = this->isBroken();
    if ( obool.has_value() ) { jto[system].emplace( "broken", *obool ); }
    else { jto[system].emplace( "broken", nlohmann::json() ); }

    obool = this->isUnfree();
    if ( obool.has_value() ) { jto[system].emplace( "unfree", *obool ); }
    else { jto[system].emplace( "unfree", nlohmann::json() ); }

    if ( withDescription )
      {
        std::optional<std::string> odesc = this->getDescription();
        if ( odesc.has_value() )
          {
            jto[system].emplace( "description", *odesc );
          }
        else { jto[system].emplace( "description", nlohmann::json() ); }
      }

    return jto;
  }


  /* --------------------------------------------------------------------------
   */

}; /* End class `Package' */


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
