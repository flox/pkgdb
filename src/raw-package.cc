/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/raw-package.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

RawPackage::RawPackage( const nlohmann::json & drvInfo )
  : _pathS()
  , _fullname( drvInfo.at( "name" ) )
  , _pname( drvInfo.at( "pname" ) )
  , _version( drvInfo.at( "version" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "version" ).get<std::string>() )
            )
  , _semver( drvInfo.at( "semver" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "semver" ).get<std::string>() )
            )
  , _license( drvInfo.at( "license" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "license" ).get<std::string>() )
            )
  , _outputs( drvInfo.at( "outputs" ) )
  , _outputsToInstall( drvInfo.at( "outputsToInstall" ) )
  , _broken( drvInfo.at( "broken" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "broken" ).get<bool>() )
            )
  , _unfree( drvInfo.at( "unfree" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "unfree" ).get<bool>() )
            )
  , _hasMetaAttr( drvInfo.at( "hasMetaAttr" ).get<bool>() )
  , _hasPnameAttr( drvInfo.at( "hasPnameAttr" ).get<bool>() )
  , _hasVersionAttr( drvInfo.at( "hasVersionAttr" ).get<bool>() )
{
  this->_pathS.push_back( drvInfo["subtree"] );
  this->_pathS.push_back( drvInfo["system"] );
  for ( auto & p : drvInfo["path"] ) { this->_pathS.push_back( p ); }
}


/* -------------------------------------------------------------------------- */

RawPackage::RawPackage( nlohmann::json && drvInfo )
  : _pathS()
  , _fullname( std::move( drvInfo.at( "name" ) ) )
  , _pname( std::move( drvInfo.at( "pname" ) ) )
  , _version( drvInfo.at( "version" ).is_null()
              ? std::nullopt
              : std::make_optional(
                  std::move( drvInfo.at( "version" ).get<std::string>() )
                )
            )
  , _semver( drvInfo.at( "semver" ).is_null()
              ? std::nullopt
              : std::make_optional(
                  std::move( drvInfo.at( "semver" ).get<std::string>() )
                )
            )
  , _license( drvInfo.at( "license" ).is_null()
              ? std::nullopt
              : std::make_optional(
                  std::move( drvInfo.at( "license" ).get<std::string>() )
                )
            )
  , _outputs( std::move( drvInfo.at( "outputs" ) ) )
  , _outputsToInstall( std::move( drvInfo.at( "outputsToInstall" ) ) )
  , _broken( drvInfo.at( "broken" ).is_null()
              ? std::nullopt
              : std::make_optional(
                  std::move( drvInfo.at( "broken" ).get<bool>() )
                )
            )
  , _unfree( drvInfo.at( "unfree" ).is_null()
              ? std::nullopt
              : std::make_optional(
                  std::move( drvInfo.at( "unfree" ).get<bool>() )
                )
            )
  , _hasMetaAttr( std::move( drvInfo.at( "hasMetaAttr" ).get<bool>() ) )
  , _hasPnameAttr( std::move( drvInfo.at( "hasPnameAttr" ).get<bool>() ) )
  , _hasVersionAttr( std::move( drvInfo.at( "hasVersionAttr" ).get<bool>() ) )
{
  this->_pathS.push_back( std::move( drvInfo["subtree"] ) );
  this->_pathS.push_back( std::move( drvInfo["system"] ) );
  for ( auto & p : drvInfo["path"] )
    {
      this->_pathS.push_back( std::move( p ) );
    }
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
