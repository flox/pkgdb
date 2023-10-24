/* ========================================================================== *
 *
 * @file pkgdb/params.cc
 *
 * @brief User settings used to query a package database.
 *
 *
 * -------------------------------------------------------------------------- */

#include <map>
#include <nix/config.hh>
#include <nix/globals.hh>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "flox/pkgdb/params.hh"
#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

void
QueryPreferences::clear()
{
  this->systems                  = { nix::settings.thisSystem.get() };
  this->allow.unfree             = true;
  this->allow.broken             = false;
  this->allow.licenses           = std::nullopt;
  this->semver.preferPreReleases = false;
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
QueryPreferences::fillPkgQueryArgs( pkgdb::PkgQueryArgs &pqa ) const
{
  pqa.clear();
  pqa.systems           = this->systems;
  pqa.allowUnfree       = this->allow.unfree;
  pqa.allowBroken       = this->allow.broken;
  pqa.licenses          = this->allow.licenses;
  pqa.preferPreReleases = this->semver.preferPreReleases;
  return pqa;
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, QueryPreferences &prefs )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw ParseQueryPreferencesException(
        "preferences must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }
  prefs.clear();
  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "systems" )
        {
          if ( value.is_null() ) { continue; }
          try
            {
              value.get_to( prefs.systems );
            }
          catch ( nlohmann::json::exception &e )
            {
              throw ParseQueryPreferencesException(
                "couldn't interpret preferences field 'systems'",
                extract_json_errmsg( e ).c_str() );
            }
        }
      else if ( key == "allow" )
        {
          if ( value.is_null() ) { continue; }
          for ( const auto &[akey, avalue] : value.items() )
            {
              if ( akey == "unfree" )
                {
                  if ( avalue.is_null() ) { continue; }

                  try
                    {
                      avalue.get_to( prefs.allow.unfree );
                    }
                  catch ( nlohmann::json::exception &e )
                    {
                      throw ParseQueryPreferencesException(
                        "couldn't interpret preferences "
                        "field 'allow.unfree'",
                        extract_json_errmsg( e ).c_str() );
                    }
                }
              else if ( akey == "broken" )
                {
                  if ( avalue.is_null() ) { continue; }
                  try
                    {
                      avalue.get_to( prefs.allow.broken );
                    }
                  catch ( nlohmann::json::exception &e )
                    {
                      throw ParseQueryPreferencesException(
                        "couldn't interpret preferences "
                        "field 'allow.broken'",
                        extract_json_errmsg( e ).c_str() );
                    }
                }
              else if ( akey == "licenses" )
                {
                  if ( avalue.is_null() ) { continue; }
                  try
                    {
                      avalue.get_to( prefs.allow.licenses );
                    }
                  catch ( nlohmann::json::exception &e )
                    {
                      throw ParseQueryPreferencesException(
                        "couldn't interpret preferences "
                        "field 'allow.licenses'",
                        extract_json_errmsg( e ).c_str() );
                    }
                }
              else
                {
                  throw ParseQueryPreferencesException(
                    "unexpected preferences field 'allow." + akey + '\'' );
                }
            }
        }
      else if ( key == "semver" )
        {
          if ( value.is_null() ) { continue; }
          for ( const auto &[skey, svalue] : value.items() )
            {
              if ( skey == "preferPreReleases" )
                {
                  if ( svalue.is_null() ) { continue; }
                  try
                    {
                      svalue.get_to( prefs.semver.preferPreReleases );
                    }
                  catch ( nlohmann::json::exception &e )
                    {
                      throw ParseQueryPreferencesException(
                        "couldn't interpret preferences field "
                        "'semver.preferPreReleases'",
                        extract_json_errmsg( e ).c_str() );
                    }
                }
              else
                {
                  throw ParseQueryPreferencesException(
                    "unexpected preferences field 'semver." + skey + '\'' );
                }
            }
        }
    }
}


void
to_json( nlohmann::json &jto, const QueryPreferences &prefs )
{
  jto = { { "systems", prefs.systems },
          { "allow",
            nlohmann::json { { "unfree", prefs.allow.unfree },
                             { "broken", prefs.allow.broken },
                             { "licenses", prefs.allow.licenses } } },
          { "semver",
            nlohmann::json {
              { "preferPreReleases", prefs.semver.preferPreReleases } } } };
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
