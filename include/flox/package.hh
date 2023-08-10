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
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/names.hh>

#include "flox/exceptions.hh"
#include "flox/types.hh"
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
    virtual std::vector<std::string_view>    getPathStrs()         const = 0;
    virtual std::string_view                 getFullName()         const = 0;
    virtual std::string_view                 getPname()            const = 0;
    virtual std::optional<std::string_view>  getVersion()          const = 0;
    virtual std::optional<std::string_view>  getLicense()          const = 0;
    virtual std::vector<std::string_view>    getOutputs()          const = 0;
    virtual std::vector<std::string_view>    getOutputsToInstall() const = 0;
    virtual std::optional<bool>              isBroken()            const = 0;
    virtual std::optional<bool>              isUnfree()            const = 0;
    virtual std::optional<std::string>       getDescription()      const = 0;

      virtual subtree_type
    getSubtreeType() const
    {
      std::vector<std::string_view> pathS = this->getPathStrs();
      if ( pathS[0] == "legacyPackages" ) { return ST_LEGACY;   }
      if ( pathS[0] == "packages" )       { return ST_PACKAGES; }
      if ( pathS[0] == "catalog" )        { return ST_CATALOG;  }
      std::string msg( "Package::getSubtreeType(): Unrecognized subtree '" );
      msg += pathS[0];
      msg += "'.";
      throw FloxException( msg );
    }

      virtual std::optional<std::string_view>
    getStability() const
    {
      if ( this->getSubtreeType() != ST_CATALOG ) { return std::nullopt; }
      return this->getPathStrs()[2];
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
      virtual std::string_view
    getPkgAttrName() const
    {
      std::vector<std::string_view> pathS = this->getPathStrs();
      if ( this->getSubtreeType() == ST_CATALOG )
        {
          return pathS[pathS.size() - 2];
        }
      return pathS[pathS.size() - 1];
    }

    /**
     * @return `std::nullopt` iff @a this package does not use semantic
     *         versioning, otherwise a normalized semantic version number
     *         coerces from @a this package's `version` information.
     */
      virtual std::optional<std::string_view>
    getSemver() const
    {
      std::optional<std::string_view> version = this->getVersion();
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
      std::string uri                     = ref.to_string() + "#";
      std::vector<std::string_view> pathS = this->getPathStrs();
      for ( size_t i = 0; i < pathS.size(); ++i )
        {
          uri += '"';
          uri += pathS[i];
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
      std::string_view system = this->getPathStrs()[1];
      nlohmann::json j = { { system, {
        { "name",  this->getFullName() }
      , { "pname", this->getPname() }
      } } };
      std::optional<std::string_view> os = this->getVersion();

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
