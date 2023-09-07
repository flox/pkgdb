/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/raw-package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

RawPackage::RawPackage( const nlohmann::json & drvInfo )
  : _pathS( drvInfo.contains( "pathS" )
            ? drvInfo["pathS"].get<std::vector<std::string>>()
            : std::vector<std::string> {}
          )
  , _fullname( drvInfo.at( "name" ) )
  , _pname( drvInfo.at( "pname" ) )
  , _version( drvInfo.contains( "version" )
              ? std::make_optional( drvInfo.at( "version" ).get<std::string>() )
              : std::nullopt
            )
  , _semver( drvInfo.contains( "semver" )
              ? std::make_optional( drvInfo.at( "semver" ).get<std::string>() )
              : std::nullopt
            )
  , _license( drvInfo.contains( "license" )
              ? std::make_optional( drvInfo.at( "license" ).get<std::string>() )
              : std::nullopt
            )
  , _outputs( drvInfo.contains( "outputs" )
              ? drvInfo["outputs"].get<std::vector<std::string>>()
              : std::vector<std::string> {}
            )
  , _outputsToInstall(
      drvInfo.contains( "outputsToInstall" )
      ? drvInfo["outputsToInstall"].get<std::vector<std::string>>()
      : std::vector<std::string> {}
    )
  , _broken( drvInfo.contains( "broken" )
              ? std::make_optional( drvInfo.at( "broken" ).get<bool>() )
              : std::nullopt
            )
  , _unfree( drvInfo.contains( "unfree" )
              ? std::make_optional( drvInfo.at( "unfree" ).get<bool>() )
              : std::nullopt
            )
  , _description( drvInfo.contains( "description" )
              ? std::make_optional( drvInfo["description"].get<std::string>() )
              : std::nullopt
            )
{}


/* -------------------------------------------------------------------------- */

RawPackage::RawPackage( nlohmann::json && drvInfo )
  : _pathS( drvInfo.contains( "pathS" )
            ? drvInfo["pathS"].get<std::vector<std::string>>()
            : std::vector<std::string> {}
          )
  , _fullname( std::move( drvInfo.at( "name" ) ) )
  , _pname( std::move( drvInfo.at( "pname" ) ) )
  , _version( drvInfo.contains( "version" )
              ? std::make_optional( drvInfo.at( "version" ).get<std::string>() )
              : std::nullopt
            )
  , _semver( drvInfo.contains( "semver" )
              ? std::make_optional( drvInfo.at( "semver" ).get<std::string>() )
              : std::nullopt
            )
  , _license( drvInfo.contains( "license" )
              ? std::make_optional( drvInfo.at( "license" ).get<std::string>() )
              : std::nullopt
            )
  , _outputs( drvInfo.contains( "outputs" )
              ? drvInfo["outputs"].get<std::vector<std::string>>()
              : std::vector<std::string> {}
            )
  , _outputsToInstall(
      drvInfo.contains( "outputsToInstall" )
      ? drvInfo["outputsToInstall"].get<std::vector<std::string>>()
      : std::vector<std::string> {}
    )
  , _broken( drvInfo.contains( "broken" )
              ? std::make_optional( drvInfo.at( "broken" ).get<bool>() )
              : std::nullopt
            )
  , _unfree( drvInfo.contains( "unfree" )
              ? std::make_optional( drvInfo.at( "unfree" ).get<bool>() )
              : std::nullopt
            )
  , _description( drvInfo.contains( "description" )
              ? std::make_optional( drvInfo["description"].get<std::string>() )
              : std::nullopt
            )
{}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
