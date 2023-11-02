/* ========================================================================== *
 *
 * @file resolver/lockfile.cc
 *
 * @brief A lockfile representing a resolved environment.
 *
 * This lockfile is processed by `mkEnv` to realize an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, LockedInputRaw &raw )
{
  if ( ! jfrom.is_object() )
    {
      // TODO: real error handling
      throw FloxException( "locked input must be an object, but is a "
                           + std::string( jfrom.type_name() ) + '.' );
    }

  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "fingerprint" )
        {
          try
            {
              raw.fingerprint = pkgdb::Fingerprint::parseNonSRIUnprefixed(
                value.get<std::string>(),
                nix::htSHA256 );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse locked input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else if ( key == "url" )
        {
          try
            {
              value.get_to( raw.url );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse locked input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else if ( key == "attrs" )
        {
          try
            {
              value.get_to( raw.attrs );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse locked input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else
        {
          throw FloxException( "encountered unexpected field `" + key
                               + "' while parsing locked input" );
        }
    }
}


void
to_json( nlohmann::json &jto, const LockedInputRaw &raw )
{
  jto = { { "fingerprint", raw.fingerprint.to_string( nix::Base16, false ) },
          { "url", raw.url },
          { "attrs", raw.attrs } };
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, LockedPackageRaw &raw )
{
  if ( ! jfrom.is_object() )
    {
      // TODO: real error handling
      throw FloxException( "locked package must be an object, but is a "
                           + std::string( jfrom.type_name() ) + '.' );
    }

  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "input" )
        {
          try
            {
              value.get_to( raw.input );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse package input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else if ( key == "attr-path" )
        {
          try
            {
              value.get_to( raw.attrPath );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse package input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else if ( key == "priority" )
        {
          try
            {
              value.get_to( raw.priority );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw FloxException( "couldn't parse package input field '" + key
                                     + "'",
                                   extract_json_errmsg( err ).c_str() );
            }
        }
      else if ( key == "info" ) { raw.info = value; }
      else
        {
          throw FloxException( "encountered unexpected field `" + key
                               + "' while parsing locked package" );
        }
    }
}


void
to_json( nlohmann::json &jto, const LockedPackageRaw &raw )
{
  jto = { { "input", raw.input },
          { "attr-path", raw.attrPath },
          { "priority", raw.priority },
          { "info", raw.info } };
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
