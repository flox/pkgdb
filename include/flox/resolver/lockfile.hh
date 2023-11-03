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

struct LockfileRaw
{

  ManifestRaw                                manifest;
  RegistryRaw                                registry;
  std::unordered_map<System, SystemPackages> packages;
  unsigned                                   lockfileVersion = 0;


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

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
