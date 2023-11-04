/* ========================================================================== *
 *
 * @file flox/resolver/lockfile.hh
 *
 * @brief A lockfile representing a resolved environment.
 *
 * This lockfile is processed by `mkEnv` to realize an environment.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <unordered_map>

#include <nlohmann/json.hpp>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/pkgdb/read.hh"
#include "flox/registry.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @class flox::resolver::InvalidLockfileException
 * @brief An exception thrown when a lockfile is invalid.
 * @{
 */
FLOX_DEFINE_EXCEPTION( InvalidLockfileException,
                       EC_INVALID_LOCKFILE,
                       "invalid lockfile" )
/** @} */


/* -------------------------------------------------------------------------- */

struct LockedInputRaw
{

  pkgdb::Fingerprint fingerprint; /**< Unique hash of associated flake. */
  std::string        url;         /**< Locked URI string.  */
  /** Exploded form of URI as an attr-set. */
  nlohmann::json attrs;

  LockedInputRaw() : fingerprint( nix::htSHA256 ) {}
  ~LockedInputRaw() = default;

  explicit LockedInputRaw( const pkgdb::PkgDbReadOnly & pdb )
    : fingerprint( pdb.fingerprint )
    , url( pdb.lockedRef.string )
    , attrs( pdb.lockedRef.attrs )
  {}

  explicit LockedInputRaw( const pkgdb::PkgDbInput & input )
    : LockedInputRaw( *input.getDbReadOnly() )
  {}


}; /* End struct `LockedInputRaw::Input' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::LockedInputRaw. */
void
from_json( const nlohmann::json & jfrom, LockedInputRaw & raw );

/** @brief Convert a @a flox::resolver::LockedInputRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const LockedInputRaw & raw );


/* -------------------------------------------------------------------------- */

/** @brief A locked package's _installable URI_. */
struct LockedPackageRaw
{
  LockedInputRaw input;
  AttrPath       attrPath;
  unsigned       priority;
  nlohmann::json info; /* pname, version, license */
};                     /* End struct `LockedPackageRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::LockedPackageRaw. */
void
from_json( const nlohmann::json & jfrom, LockedPackageRaw & raw );

/** @brief Convert a @a flox::resolver::LockedPackageRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const LockedPackageRaw & raw );


/* -------------------------------------------------------------------------- */

using SystemPackages = std::unordered_map<InstallID, LockedPackageRaw>;

/**
 * @brief An environment lockfile in its _raw_ form.
 *
 * This form is suitable for _instantiating_ ( _i.e._, realizing ) an
 * environment using `mkEnv`.
 */
struct LockfileRaw
{

  ManifestRaw                                manifest;
  RegistryRaw                                registry;
  std::unordered_map<System, SystemPackages> packages;
  unsigned                                   lockfileVersion = 0;


  ~LockfileRaw()                     = default;
  LockfileRaw()                      = default;
  LockfileRaw( const LockfileRaw & ) = default;
  LockfileRaw( LockfileRaw && )      = default;
  LockfileRaw &
  operator=( const LockfileRaw & )
    = default;
  LockfileRaw &
  operator=( LockfileRaw && )
    = default;

  /**
   * @brief Check the lockfile for validity, throw and exception if it
   *        is invalid.
   */
  void
  check() const
  {
    // TODO: Implement in `lockfile.cc'.
  }

  /** @brief Reset to default/empty state. */
  void
  clear();


}; /* End struct `LockfileRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::LockfileRaw. */
void
from_json( const nlohmann::json & jfrom, LockfileRaw & raw );

/** @brief Convert a @a flox::resolver::LockfileRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const LockfileRaw & raw );


/* -------------------------------------------------------------------------- */

/**
 * @brief A locked representation of an environment.
 *
 * Unlike the _raw_ form, this form is suitable for stashing temporary variables
 * and other information that is not needed for serializing/de-serializing.
 */
class Lockfile
{

private:

  std::filesystem::path lockfilePath;
  LockfileRaw           lockfileRaw;
  /** Contains the locked registry if one is present, otherwise empty. */
  RegistryRaw registryRaw;
  /** Maps `{ <INSTALL-ID>: <INPUT> }` for all `packages` members. */
  RegistryRaw packagesRegistryRaw;


  /**
   * @brief Initialize @a registryRaw and @a packagesRegistryRaw members
   *        from @a lockfileRaw. */
  void
  init();


public:

  ~Lockfile()                  = default;
  Lockfile()                   = default;
  Lockfile( const Lockfile & ) = default;
  Lockfile( Lockfile && )      = default;

  Lockfile( std::filesystem::path lockfilePath, LockfileRaw raw )
    : lockfilePath( std::move( lockfilePath ) ), lockfileRaw( std::move( raw ) )
  {
    this->init();
  }

  explicit Lockfile( std::filesystem::path lockfilePath );

  Lockfile &
  operator=( const Lockfile & )
    = default;

  Lockfile &
  operator=( Lockfile && )
    = default;


}; /* End class `Lockfile' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
