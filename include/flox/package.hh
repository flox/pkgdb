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
#include <unordered_map>
#include <unordered_set>

#include "flox/types.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/**
 * Abstract representation of a "package", analogous to a Nix `derivation'.
 * This abstraction provides a common base for various backends that store,
 * evaluate, and communicate package definitions.
 *
 * Notable Implementation of this interface include:
 * - @a RawPackage    : Comprised of raw C++ values, often used for testing.
 * - @a CachedPackage : A package definition stored in a SQL database.
 * - @a FlakePackage  : A package definition evaluated from a Nix `flake'.
 */
class Package {
  public:
    virtual std::vector<std::string>    getPathStrs()         const = 0;
    virtual std::string                 getFullName()         const = 0;
    virtual std::string                 getPname()            const = 0;
    virtual std::optional<std::string>  getVersion()          const = 0;
    virtual std::optional<std::string>  getLicense()          const = 0;
    virtual std::vector<std::string>    getOutputs()          const = 0;
    virtual std::vector<std::string>    getOutputsToInstall() const = 0;
    virtual std::optional<bool>         isBroken()            const = 0;
    virtual std::optional<bool>         isUnfree()            const = 0;
    virtual bool                        hasMetaAttr()         const = 0;
    virtual bool                        hasPnameAttr()        const = 0;
    virtual bool                        hasVersionAttr()      const = 0;

      virtual subtree_type
    getSubtreeType() const
    {
      std::vector<std::string> pathS = this->getPathStrs();
      if ( pathS[0] == "legacyPackages" ) { return ST_LEGACY;   }
      if ( pathS[0] == "packages" )       { return ST_PACKAGES; }
      if ( pathS[0] == "catalog" )        { return ST_PACKAGES; }
      throw ResolverException(
        "Package::getSubtreeType(): Unrecognized subtree '" + pathS[0] + "'."
      );
    }

      virtual std::optional<std::string>
    getStability() const
    {
      if ( this->getSubtreeType() != ST_CATALOG ) { return std::nullopt; }
      return this->getPathStrs()[2];
    }

      virtual nix::DrvName
    getParsedDrvName() const
    {
      return nix::DrvName( this->getFullName() );
    }

      virtual std::string
    getPkgAttrName() const
    {
      std::vector<std::string> pathS = this->getPathStrs();
      if ( this->getSubtreeType() == ST_CATALOG )
        {
          return pathS[pathS.size() - 2];
        }
      return pathS[pathS.size() - 1];
    }

      virtual std::optional<std::string>
    getSemver() const
    {
      std::optional<std::string> version = this->getVersion();
      if ( ! version.has_value() ) { return std::nullopt; }
      return versions::coerceSemver( version.value() );
    }

      virtual std::string
    toURIString( const FloxFlakeRef & ref ) const
    {
      std::string uri                = ref.to_string() + "#";
      std::vector<std::string> pathS = this->getPathStrs();
      for ( size_t i = 0; i < pathS.size(); ++i )
        {
          uri += "\"" + pathS[i] + "\"";
          if ( ( i + 1 ) < pathS.size() ) uri += ".";
        }
      return uri;
    }

      virtual nlohmann::json
    getInfo() const
    {
      std::string system = this->getPathStrs()[1];
      nlohmann::json j = { { system, {
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

      return j;
    }


/* -------------------------------------------------------------------------- */

};  /* End class `Package' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
