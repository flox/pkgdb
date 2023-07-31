/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/exceptions.hh"
#include "flox/flake-package.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  void
FlakePackage::init( bool checkDrv )
{
  if ( this->_path.size() < 3 )
    {
      throw FloxException(
        "Package::init(): Package attribute paths must have at least 3 "
        "elements - the path '" + this->_cursor->getAttrPathStr() +
        "' is too short."
      );
    }

  if ( checkDrv && ( ! this->_cursor->isDerivation() ) )
    {
      throw FloxException(
        "Package::init(): Packages must be derivations but the attrset at '"
        + this->_cursor->getAttrPathStr() +
        "' does not set `.type = \"derivation\"'."
      );
    }

  /* Subtree type */
  if ( this->_pathS[0] == "packages" )
    {
      this->_subtree = ST_PACKAGES;
    }
  else if ( this->_pathS[0] == "catalog" )
    {
      this->_subtree = ST_CATALOG;
    }
  else if ( this->_pathS[0] == "legacyPackages" )
    {
      this->_subtree = ST_LEGACY;
    }
  else
    {
      throw FloxException(
        "FlakePackage::init(): Invalid subtree name '" + this->_pathS[0] +
        "' at path '" + this->_cursor->getAttrPathStr() + "'."
      );
    }

  this->_system = this->_pathS[1];

  MaybeCursor c = this->_cursor->maybeGetAttr( "meta" );
  this->_hasMetaAttr = c != nullptr;

  c = this->_cursor->maybeGetAttr( "pname" );
  if ( c != nullptr )
    {
      try
        {
          this->_pname        = c->getString();
          this->_hasPnameAttr = true;
        }
      catch( ... ) {}
    }

  /* Version and Semver */
  c = this->_cursor->maybeGetAttr( "version" );
  if ( c != nullptr )
    {
      std::string v;
      try
        {
          this->_version        = c->getString();
          this->_hasVersionAttr = true;
        }
      catch( ... ) {}
    }

  if ( ! this->_version.empty() )
    {
      this->_semver = versions::coerceSemver( this->_version );
    }
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string_view>
FlakePackage::getOutputsToInstall() const
{
  if ( this->_hasMetaAttr )
    {
      MaybeCursor m =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "outputsToInstall" );
      if ( m != nullptr )
        {
          std::vector<std::string_view> rsl;
          for ( const auto & o : m->getListOfStrings() )
            {
              rsl.emplace_back( o );
            }
          return rsl;
        }
    }
  std::vector<std::string_view> rsl;
  for ( std::string_view o : this->getOutputs() )
    {
      rsl.push_back( o );
      if ( o == "out" ) { break; }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::optional<bool>
FlakePackage::isBroken() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  try
    {
      MaybeCursor b =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "broken" );
      if ( b == nullptr ) { return std::nullopt; }
      return b->getBool();
    }
  catch( ... )
    {
      return std::nullopt;
    }
}

  std::optional<bool>
FlakePackage::isUnfree() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  try
    {
      MaybeCursor u =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "unfree" );
      if ( u == nullptr ) { return std::nullopt; }
      return u->getBool();
    }
  catch( ... )
    {
      return std::nullopt;
    }
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
