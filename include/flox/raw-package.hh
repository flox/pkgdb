/* ========================================================================== *
 *
 * @file flox/raw-package.hh
 *
 * @brief The simplest `Package' implementation comprised of raw values.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/package.hh"
#include <nix/flake/flake.hh>
#include <nlohmann/json.hpp>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * @brief The simplest `Package' implementation comprised of raw values.
 *
 * This form largely exists for testing purposes.
 */
class RawPackage : public Package
{

public:

  AttrPath                   path;
  std::string                name;
  std::string                pname;
  std::optional<std::string> version;
  std::optional<std::string> semver;
  std::optional<std::string> license;
  std::vector<std::string>   outputs;
  std::vector<std::string>   outputsToInstall;
  std::optional<bool>        broken;
  std::optional<bool>        unfree;
  std::optional<std::string> description;

  RawPackage( const AttrPath &                 path             = {},
              std::string_view                 name             = {},
              std::string_view                 pname            = {},
              std::optional<std::string>       version          = std::nullopt,
              std::optional<std::string>       semver           = std::nullopt,
              std::optional<std::string>       license          = std::nullopt,
              const std::vector<std::string> & outputs          = { "out" },
              const std::vector<std::string> & outputsToInstall = { "out" },
              std::optional<bool>              broken           = std::nullopt,
              std::optional<bool>              unfree           = std::nullopt,
              std::optional<std::string>       description      = std::nullopt )
    : path( path )
    , name( name )
    , pname( pname )
    , version( version )
    , semver( semver )
    , license( license )
    , outputs( outputs )
    , outputsToInstall( outputsToInstall )
    , broken( broken )
    , unfree( unfree )
    , description( description )
  {}


  /* --------------------------------------------------------------------------
   */

  AttrPath
  getPathStrs() const override
  {
    return this->path;
  }

  std::string
  getFullName() const override
  {
    return this->name;
  }

  std::string
  getPname() const override
  {
    return this->pname;
  }

  std::optional<std::string>
  getVersion() const override
  {
    return this->version;
  }

  std::optional<std::string>
  getSemver() const override
  {
    return this->semver;
  }

  std::optional<std::string>
  getLicense() const override
  {
    return this->license;
  }

  std::vector<std::string>
  getOutputs() const override
  {
    return this->outputs;
  }

  std::vector<std::string>
  getOutputsToInstall() const override
  {
    return this->outputsToInstall;
  }

  std::optional<bool>
  isBroken() const override
  {
    return this->broken;
  }

  std::optional<bool>
  isUnfree() const override
  {
    return this->unfree;
  }

  std::optional<std::string>
  getDescription() const override
  {
    return this->description;
  }

}; /* End class `RawPackage' */


/** @brief Convert a JSON object to a @a flox::RawPackage. */
void
from_json( const nlohmann::json & jfrom, RawPackage & pkg )
{
  assertIsJSONObject<flox::pkgdb::PkgDbException>( jfrom, "package" );
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "path" )
        {
          try
            {
              value.get_to( pkg.path );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `path'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "name" )
        {
          try
            {
              value.get_to( pkg.name );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `name'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "pname" )
        {
          try
            {
              value.get_to( pkg.pname );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `pname'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "version" )
        {
          try
            {
              value.get_to( pkg.version );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `version'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "semver" )
        {
          try
            {
              value.get_to( pkg.semver );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `semver'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "license" )
        {
          try
            {
              value.get_to( pkg.license );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `license'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "outputs" )
        {
          try
            {
              value.get_to( pkg.outputs );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `outputs'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "outputsToInstall" )
        {
          try
            {
              value.get_to( pkg.outputsToInstall );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `outputsToInstall'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "broken" )
        {
          try
            {
              value.get_to( pkg.broken );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `broken'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "unfree" )
        {
          try
            {
              value.get_to( pkg.unfree );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `unfree'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else if ( key == "description" )
        {
          try
            {
              value.get_to( pkg.description );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw flox::pkgdb::PkgDbException(
                "couldn't interpret field `description'",
                flox::extract_json_errmsg( e ) );
            }
        }
      else
        {
          throw flox::pkgdb::PkgDbException( "unrecognized field `" + key
                                             + "'" );
        }
    }
}

/** @brief Convert a @a flox::RawPackage to a JSON object. */
void
to_json( nlohmann::json & jto, const flox::RawPackage & pkg )
{
  jto = { { "path", pkg.path },
          {
            "name",
            pkg.name,
          },
          { "pname", pkg.pname },
          { "version", pkg.version },
          { "semver", pkg.semver },
          { "license", pkg.license },
          { "outputs", pkg.outputs },
          { "outputsToInstall", pkg.outputsToInstall },
          { "broken", pkg.broken },
          { "unfree", pkg.unfree },
          { "description", pkg.description } };
}

/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
