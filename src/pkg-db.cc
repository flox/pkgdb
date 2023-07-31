/* ========================================================================== *
 *
 * @file pkg-db.cc
 *
 * @brief Implementations for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#include "pkg-db.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
  std::string
getPkgDbName( const Fingerprint & fingerprint )
{
  nix::Path   cacheDir = nix::getCacheDir() + "/flox/pkgdb-v0";
  std::string fpStr    = fingerprint.to_string( nix::Base16, false );
  nix::Path   dbPath   = cacheDir + "/" + fpStr + ".sqlite";
  return dbPath;
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::initTables()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  void
PkgDb::loadLockedRef()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDb::getDbVersion()
{
  // TODO
  return "";
}


/* -------------------------------------------------------------------------- */

  int
PkgDb::execute( std::string_view stmt )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  bool
PkgDb::hasPackageSet( const AttrPathV & path )
{
  // TODO
  return false;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::getPackageSetId( const AttrPathV & path )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  std::string
PkgDb::getDescription( row_id descriptionId )
{
  // TODO
  return "";
}


/* -------------------------------------------------------------------------- */

  bool
PkgDb::hasPackage( const AttrPathV & path )
{
  // TODO
  return false;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetPackageSetId( const AttrPathV & path )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addOrGetDescriptionId( std::string_view description )
{
  // TODO
  return 0;
}

/* -------------------------------------------------------------------------- */

  row_id
PkgDb::addPackage( row_id           parentId
                 , std::string_view attrName
                 , Cursor           cursor
                 , bool             replace
                 )
{
  // TODO
  return 0;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
