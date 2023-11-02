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

#include "flox/core/types.hh"
#include "flox/pkgdb/read.hh"
#include "flox/registry.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

struct LockedInputRaw
{
  pkgdb::Fingerprint fingerprint; /**< Unique hash of associated flake. */
  std::string        url;         /**< Locked URI string.  */
  /** Exploded form of URI as an attr-set. */
  nlohmann::json attrs;

  explicit LockedInputRaw( const pkgdb::PkgDbReadOnly & pdb )
    : fingerprint( pdb.fingerprint )
    , url( pdb.lockedRef.string )
    , attrs( pdb.lockedRef.attrs )
  {}

  explicit LockedInputRaw( const pkgdb::PkgDbInput & input )
    : LockedInputRaw( *input.getDbReadOnly() )
  {}


}; /* End struct `LockedInputRaw::Input' */


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


/** @brief Convert a JSON object to a @a flox::resolver::LockedPackageRaw. */
void
from_json( const nlohmann::json & jfrom, LockedPackageRaw & raw );

/** @brief Convert a @a flox::resolver::LockedPackageRaw to a JSON object. */
void
to_json( nlohmann::json & jto, const LockedPackageRaw & raw );


/* -------------------------------------------------------------------------- */

using SystemPackages = std::unordered_map<std::string, LockedPackageRaw>;

struct LockfileRaw
{
  ManifestRaw                                     manifest;
  RegistryRaw                                     registry;
  std::unordered_map<std::string, SystemPackages> packages;
  unsigned                                        lockfileVersion = 0;
}; /* End struct `LockfileRaw' */


// FIXME
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( LockfileRaw,
//                                    manifest,
//                                    registry,
//                                    packages,
//                                    lockfileVersion )


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
