/* ========================================================================== *
 *
 * @file pkgdb/params.cc
 *
 * @brief User settings used to query a package database.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/globals.hh>

#include "flox/core/util.hh"
#include "flox/resolver/params.hh"


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
QueryPreferences::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
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
from_json( const nlohmann::json & jfrom, QueryPreferences & prefs )
{
  prefs.clear();
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "systems" )
        {
          if ( value.is_null() ) { continue; }
          value.get_to( prefs.systems );
        }
      else if ( key == "allow" )
        {
          if ( value.is_null() ) { continue; }
          for ( const auto & [akey, avalue] : value.items() )
            {
              if ( akey == "unfree" )
                {
                  if ( avalue.is_null() ) { continue; }
                  avalue.get_to( prefs.allow.unfree );
                }
              else if ( akey == "broken" )
                {
                  if ( avalue.is_null() ) { continue; }
                  avalue.get_to( prefs.allow.broken );
                }
              else if ( akey == "licenses" )
                {
                  if ( avalue.is_null() ) { continue; }
                  avalue.get_to( prefs.allow.licenses );
                }
              else
                {
                  throw FloxException( "Unexpected preferences field 'allow." +
                                       akey + '\'' );
                }
            }
        }
      else if ( key == "semver" )
        {
          if ( value.is_null() ) { continue; }
          for ( const auto & [skey, svalue] : value.items() )
            {
              if ( skey == "preferPreReleases" )
                {
                  if ( svalue.is_null() ) { continue; }
                  svalue.get_to( prefs.semver.preferPreReleases );
                }
              else
                {
                  throw FloxException( "Unexpected preferences field 'semver." +
                                       skey + '\'' );
                }
            }
        }
    }
}


void
to_json( nlohmann::json & jto, const QueryPreferences & prefs )
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
