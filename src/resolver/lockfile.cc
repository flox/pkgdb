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

#include <nix/hash.hh>

#include "flox/core/util.hh"
#include "flox/resolver/lockfile.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
Lockfile::check() const
{
  this->lockfileRaw.check();
}


/* -------------------------------------------------------------------------- */

void
Lockfile::init()
{
  this->lockfileRaw.check();

  /* Collect inputs from all locked packages into a registry keyed
   * by fingerprints. */
  for ( const auto &[system, sysPkgs] : this->lockfileRaw.packages )
    {
      for ( const auto &[pid, pkg] : sysPkgs )
        {
          this->packagesRegistryRaw.inputs.try_emplace(
            pkg.input.fingerprint.to_string( nix::Base16, false ),
            static_cast<RegistryInput>( pkg.input ) );
        }
    }

  this->check();
}


/* -------------------------------------------------------------------------- */

/** @brief Read a flox::resolver::Lockfile from a file. */
static LockfileRaw
readLockfileFromPath( const std::filesystem::path &lockfilePath )
{
  if ( ! std::filesystem::exists( lockfilePath ) )
    {
      throw InvalidLockfileException( "no such path: "
                                      + lockfilePath.string() );
    }
  return readAndCoerceJSON( lockfilePath );
}

/* -------------------------------------------------------------------------- */

Lockfile::Lockfile( std::filesystem::path lockfilePath )
  : lockfilePath( std::move( lockfilePath ) )
  , lockfileRaw( readLockfileFromPath( this->lockfilePath ) )
{
  this->init();
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, LockedInputRaw &raw )
{
  assertIsJSONObject<InvalidLockfileException>( jfrom, "locked input" );

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
              throw InvalidLockfileException(
                "couldn't parse locked input field `" + key + "'",
                extract_json_errmsg( err ) );
            }
          catch ( nix::BadHash &err )
            {
              throw InvalidHashException(
                "failed to parse locked input fingerprint",
                err.what() );
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
              throw InvalidLockfileException(
                "couldn't parse locked input field `" + key + "'",
                extract_json_errmsg( err ) );
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
              throw InvalidLockfileException(
                "couldn't parse locked input field `" + key + "'",
                extract_json_errmsg( err ) );
            }
        }
      else
        {
          throw InvalidLockfileException( "encountered unexpected field `" + key
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
  assertIsJSONObject<InvalidLockfileException>( jfrom, "locked package" );

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
              throw InvalidLockfileException(
                "couldn't parse package input field `" + key + "'",
                extract_json_errmsg( err ) );
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
              throw InvalidLockfileException(
                "couldn't parse package input field `" + key + "'",
                extract_json_errmsg( err ) );
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
              throw InvalidLockfileException(
                "couldn't parse package input field `" + key + "'",
                extract_json_errmsg( err ) );
            }
        }
      else if ( key == "info" ) { raw.info = value; }
      else
        {
          throw InvalidLockfileException( "encountered unexpected field `" + key
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

void
LockfileRaw::clear()
{
  this->manifest.clear();
  this->registry.clear();
  this->packages        = std::unordered_map<System, SystemPackages> {};
  this->lockfileVersion = 0;
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, LockfileRaw &raw )
{
  assertIsJSONObject<InvalidLockfileException>( jfrom, "lockfile" );
  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "manifest" )
        {
          try
            {
              value.get_to( raw.manifest );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw InvalidLockfileException( "couldn't parse lockfile field `"
                                                + key + "'",
                                              extract_json_errmsg( err ) );
            }
        }
      else if ( key == "registry" )
        {
          try
            {
              value.get_to( raw.registry );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw InvalidLockfileException( "couldn't parse lockfile field `"
                                                + key + "'",
                                              extract_json_errmsg( err ) );
            }
        }
      else if ( key == "packages" )
        {
          if ( ! value.is_object() )
            {
              assertIsJSONObject<InvalidLockfileException>(
                jfrom,
                "lockfile `packages' field" );
            }
          for ( const auto &[system, descriptors] : value.items() )
            {
              SystemPackages sysPkgs;
              for ( const auto &[pid, descriptor] : descriptors.items() )
                {
                  try
                    {
                      sysPkgs.emplace( pid,
                                       descriptor.get<LockedPackageRaw>() );
                    }
                  catch ( nlohmann::json::exception &err )
                    {
                      throw InvalidLockfileException(
                        "couldn't parse lockfile field `packages." + system
                          + "." + pid + "'",
                        extract_json_errmsg( err ) );
                    }
                }
              raw.packages.emplace( system, std::move( sysPkgs ) );
            }
        }
      else if ( key == "lockfile-version" )
        {
          try
            {
              value.get_to( raw.lockfileVersion );
            }
          catch ( nlohmann::json::exception &err )
            {
              throw InvalidLockfileException( "couldn't parse lockfile field `"
                                                + key + "'",
                                              extract_json_errmsg( err ) );
            }
        }
      else
        {
          throw InvalidLockfileException( "encountered unexpected field `" + key
                                          + "' while parsing locked package" );
        }
    }
}


void
to_json( nlohmann::json &jto, const LockfileRaw &raw )
{
  jto = { { "manifest", raw.manifest },
          { "registry", raw.registry },
          { "packages", raw.packages },
          { "lockfile-version", raw.lockfileVersion } };
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
