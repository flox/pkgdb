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
  namespace resolve {

/* -------------------------------------------------------------------------- */

class DrvDb;


/* -------------------------------------------------------------------------- */

/**
 * The simplest `Package' implementation comprised of raw values.
 * This form largely exists for testing purposes, but it also serves as the
 * basis for `CachedPackage' ( read from SQL databases ).
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
    bool                        _hasMetaAttr;
    bool                        _hasPnameAttr;
    bool                        _hasVersionAttr;


/* -------------------------------------------------------------------------- */

  public:
    friend class RawPackageSet;

    RawPackage( const nlohmann::json &  drvInfo );
    RawPackage(       nlohmann::json && drvInfo );
    RawPackage(
      const std::vector<std::string_view>   & pathS            = {}
    ,       std::string_view                  fullname         = {}
    ,       std::string_view                  pname            = {}
    ,       std::optional<std::string_view>   version          = std::nullopt
    ,       std::optional<std::string_view>   semver           = std::nullopt
    ,       std::optional<std::string_view>   license          = std::nullopt
    , const std::vector<std::string_view>   & outputs          = { "out" }
    , const std::vector<std::string_view>   & outputsToInstall = { "out" }
    ,       std::optional<bool>               broken           = std::nullopt
    ,       std::optional<bool>               unfree           = std::nullopt
    ,       bool                              hasMetaAttr      = false
    ,       bool                              hasPnameAttr     = false
    ,       bool                              hasVersionAttr   = false
    ) : _pname( pname )
      , _version( version )
      , _semver( semver )
      , _license( license )
      , _broken( broken )
      , _unfree( unfree )
      , _hasMetaAttr( hasMetaAttr )
      , _hasPnameAttr( hasPnameAttr )
      , _hasVersionAttr( hasVersionAttr )
    {
      for ( auto & s : pathS ) { this->_pathS.emplace_back( s ); }
      for ( auto & s : outputs ) { this->_outputs.emplace_back( s ); }
      for ( auto & s : outputsToInstall )
        {
          this->_outputsToInstall.emplace_back( s );
        }
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
      bool
    hasMetaAttr() const override
    {
      return this->_hasMetaAttr;
    }
      bool
    hasPnameAttr() const override
    {
      return this->_hasPnameAttr;
    }
      bool
    hasVersionAttr() const override
    {
      return this->_hasVersionAttr;
    }
      std::vector<std::string>
    getOutputsToInstall() const override
    {
      return this->_outputsToInstall;
    }


/* -------------------------------------------------------------------------- */

};  /* End class `RawPackage' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
