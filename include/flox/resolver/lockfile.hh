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


void
from_json( const nlohmann::json & j, LockedInputRaw & raw );
void
to_json( nlohmann::json & j, const LockedInputRaw & raw );


/* -------------------------------------------------------------------------- */

struct LockedPackageRaw
{
  LockedInputRaw input;
  AttrPath       attrPath;
  unsigned       priority;
  nlohmann::json info; /* pname, version, license */
};                     /* End struct `LockedPackageRaw' */


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( LockedPackageRaw,
                                    input,
                                    attrPath,
                                    priority,
                                    info )


/* -------------------------------------------------------------------------- */

struct LockfileRaw
{
  using SystemPackages = std::unordered_map<std::string, LockedPackageRaw>;

  ManifestRaw                                     manifest;
  RegistryRaw                                     registry;
  std::unordered_map<std::string, SystemPackages> packages;
  unsigned                                        lockfileVersion = 0;
}; /* End struct `LockfileRaw' */


// TODO: to_json( ManifestRaw )
// NLOHMANN_JSON_DEFINE_TYPE_NON_INTRUSIVE( LockfileRaw,
//                                         manifest,
//                                         registry,
//                                         packages,
//                                         lockfileVersion )


/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
