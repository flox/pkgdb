/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * The simplest `Package' implementation comprised of raw values.
 * This form largely exists for testing purposes.
 */
class RawPackage : public Package {

  protected:
    std::vector<std::string>    _pathS;
    std::string                 _fullname;
    std::string                 _pname;
    std::optional<std::string>  _version;
    std::optional<std::string>  _semver;
    std::optional<std::string>  _license;
    std::vector<std::string>    _outputs;
    std::vector<std::string>    _outputsToInstall;
    std::optional<bool>         _broken;
    std::optional<bool>         _unfree;
    std::optional<std::string>  _description;


/* -------------------------------------------------------------------------- */

  public:
    RawPackage( const nlohmann::json &  drvInfo );
    RawPackage(       nlohmann::json && drvInfo );
    RawPackage(
      const std::vector<std::string>   & pathS            = {}
    ,       std::string_view             fullname         = {}
    ,       std::string_view             pname            = {}
    ,       std::optional<std::string>   version          = std::nullopt
    ,       std::optional<std::string>   semver           = std::nullopt
    ,       std::optional<std::string>   license          = std::nullopt
    , const std::vector<std::string>   & outputs          = { "out" }
    , const std::vector<std::string>   & outputsToInstall = { "out" }
    ,       std::optional<bool>          broken           = std::nullopt
    ,       std::optional<bool>          unfree           = std::nullopt
    ,       std::optional<std::string>   description      = std::nullopt
    ) : _pathS( pathS )
      , _fullname( fullname )
      , _pname( pname )
      , _version( version )
      , _semver( semver )
      , _license( license )
      , _outputs( outputs )
      , _outputsToInstall( outputsToInstall )
      , _broken( broken )
      , _unfree( unfree )
      , _description( description )
    {
    }


/* -------------------------------------------------------------------------- */

      std::vector<std::string>
    getPathStrs() const override
    {
      return this->_pathS;
    }
      std::string
    getFullName() const override
    {
      return this->_fullname;
    }
      std::string
    getPname() const override
    {
      return this->_pname;
    }
      std::optional<std::string>
    getVersion() const override
    {
      return this->_version;
    }
      std::optional<std::string>
    getSemver() const override
    {
      return this->_semver;
    }
      std::optional<std::string>
    getLicense() const override
    {
      return this->_license;
    }
      std::vector<std::string>
    getOutputs() const override
    {
      return this->_outputs;
    }
      std::optional<bool>
    isBroken() const override
    {
      return this->_broken;
    }
      std::optional<bool>
    isUnfree() const override
    {
      return this->_unfree;
    }
      std::optional<std::string>
    getDescription() const override
    {
      return this->_description;
    }
      std::vector<std::string>
    getOutputsToInstall() const override
    {
      return this->_outputsToInstall;
    }


/* -------------------------------------------------------------------------- */

};  /* End class `RawPackage' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
