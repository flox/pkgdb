/* ========================================================================== *
 *
 * @file flox/package.hh
 *
 * @brief Abstract representation of a package.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>

#include <nlohmann/json.hpp>

#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/names.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Abstract representation of a "package", analogous to a Nix `derivation'.
 * This abstraction provides a common base for various backends that store,
 * evaluate, and communicate package definitions.
 */
class Package {
  public:
    /** @return attribute path where package is defined */
    virtual AttrPath getPathStrs() const = 0;

    /** @return the derivation `name` field. */
    virtual std::string getFullName() const = 0;

    /**
     * @return iff the field `pname` is defined then `pname`, otherwise the
     *         `name` field stripped of is _version_ part as recognized by
     *         `nix::DrvName` parsing rules.
     */
    virtual std::string getPname() const = 0;

    /**
     * @return iff the field `version` is defined then `version`, otherwise the
     *         `name` field stripped of is _pname_ part as recognized by
     *         `nix::DrvName` parsing rules.
     *         If `version` is undefined and `name` contains no version suffix,
     *         then `std::nullopt`.
     */
    virtual std::optional<std::string> getVersion() const = 0;

    /** @return The `meta.license.spdxId` field if defined,
     *          otherwise `std::nullopt` */
    virtual std::optional<std::string> getLicense() const = 0;

    /** @return The derivation `outputs` list. */
    virtual AttrPath getOutputs() const = 0;

    /**
     * @return The `meta.outputsToInstall` field if defined, otherwise the
     *         derivation `outputs` members to the left of and
     *         including `out`.
     */
    virtual AttrPath getOutputsToInstall() const = 0;

    /** @return The `meta.broken` field if defined, otherwise `std::nullopt`. */
    virtual std::optional<bool> isBroken() const = 0;

    /** @return The `meta.unfree` field if defined, otherwise `std::nullopt`. */
    virtual std::optional<bool> isUnfree() const = 0;

    /**
     * @return The `meta.description` field if defined,
     * otherwise `std::nullopt`.
     */
    virtual std::optional<std::string> getDescription() const = 0;

    /**
     * @return The flake `outputs` subtree the package resides in, being one of
     *         `legacyPackages`, `packages`, or `catalog`.
     */
      virtual subtree_type
    getSubtreeType() const
    {
      AttrPath pathS = this->getPathStrs();
      if ( pathS.front() == "legacyPackages" ) { return ST_LEGACY;   }
      if ( pathS.front() == "packages" )       { return ST_PACKAGES; }
      if ( pathS.front() == "catalog" )        { return ST_CATALOG;  }
      throw FloxException(
        std::string( __func__ ) + ": Unrecognized subtree '" +
        pathS.front() + "'."
      );
    }

    /**
     * @return For non-catalog packages `std::nullopt`, otherwise the catalog
     *         stability the package resides in, being one of `stable`,
     *         `staging`, or `unstable`.
     */
      virtual std::optional<std::string>
    getStability() const
    {
      if ( this->getSubtreeType() != ST_CATALOG ) { return std::nullopt; }
      return this->getPathStrs().at( 2 );
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
     * @return The attribute name assocaited with @a this package.
     *         For `catalog` packages this is the second to last member of
     *         @a this package's attribute path, for other flake subtrees.
     */
      virtual std::string
    getPkgAttrName() const
    {
      AttrPath pathS = this->getPathStrs();
      if ( this->getSubtreeType() == ST_CATALOG )
        {
          return pathS.at( pathS.size() - 2 );
        }
      return pathS.at( pathS.size() - 1 );
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
      return versions::coerceSemver( version.value() );
    }

    /**
     * Create an installable URI string associated with this package using
     * @a ref as its _input_ part.
     * @param ref Input flake reference associated with @a this package.
     *            This is used to construct the URI on the left side of `#`.
     * @return An installable URI string associated with this package using.
     */
      virtual std::string
    toURIString( const nix::FlakeRef & ref ) const
    {
      std::string uri                = ref.to_string() + "#";
      AttrPath pathS = this->getPathStrs();
      for ( size_t i = 0; i < pathS.size(); ++i )
        {
          uri += '"';
          uri += pathS.at( i );
          uri += '"';
          if ( ( i + 1 ) < pathS.size() ) uri += ".";
        }
      return uri;
    }

    /**
     * Serialize notable package metadata as a JSON object.
     * This may only contains a subset of all available information.
     * @param withDescription Whether to include `description` strings.
     * @return A JSON object with notable package metadata.
     */
      virtual nlohmann::json
    getInfo( bool withDescription = false ) const
    {
      std::string    system = this->getPathStrs().at( 1 );
      nlohmann::json j      = { { system, {
        { "name",  this->getFullName() }
      , { "pname", this->getPname() }
      } } };
      std::optional<std::string> os = this->getVersion();

      if ( os.has_value() ) { j[system].emplace( "version", os.value() ); }
      else { j[system].emplace( "version", nlohmann::json() ); }

      os = this->getSemver();
      if ( os.has_value() ) { j[system].emplace( "semver", os.value() ); }
      else { j[system].emplace( "semver", nlohmann::json() ); }

      j[system].emplace( "outputs",          this->getOutputs() );
      j[system].emplace( "outputsToInstall", this->getOutputsToInstall() );

      os = this->getLicense();
      if ( os.has_value() ) { j[system].emplace( "license", os.value() ); }
      else { j[system].emplace( "license", nlohmann::json() ); }

      std::optional<bool> ob = this->isBroken();
      if ( ob.has_value() ) { j[system].emplace( "broken", ob.value() ); }
      else { j[system].emplace( "broken", nlohmann::json() ); }

      ob = this->isUnfree();
      if ( ob.has_value() ) { j[system].emplace( "unfree", ob.value() ); }
      else { j[system].emplace( "unfree", nlohmann::json() ); }

      if ( withDescription )
        {
          std::optional<std::string> od = this->getDescription();
          if ( od.has_value() )
            {
              j[system].emplace( "description", od.value() );
            }
          else
            {
              j[system].emplace( "description", nlohmann::json() );
            }
        }

      return j;
    }


/* -------------------------------------------------------------------------- */

};  /* End class `Package' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
