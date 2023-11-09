/* ========================================================================== *
 *
 * @file search/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/core/exceptions.hh"
#include "flox/core/util.hh"
#include "flox/flox-flake.hh"
#include "flox/pkgdb/command.hh"
#include "flox/pkgdb/write.hh"
#include "flox/search/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

argparse::Argument &
SearchCommand::addSearchParamArgs( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "parameters" )
    .help( "search paramaters as inline JSON or a path to a file" )
    .required()
    .metavar( "PARAMS" )
    .action(
      [&]( const std::string & params )
      {
        nlohmann::json searchParamsRaw = parseOrReadJSONObject( params );
        searchParamsRaw.get_to( this->rawParams );
      } );
}


/* -------------------------------------------------------------------------- */

SearchCommand::SearchCommand() : parser( "search" )
{
  this->parser.add_description(
    "Search a set of flakes and emit a list satisfactory packages" );
  this->addSearchParamArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

void
SearchCommand::initEnvironment()
{
  /* Init global manifest. */
  auto maybePath = this->rawParams.getGlobalManifestPath();
  if ( maybePath.has_value() ) { this->initGlobalManifestPath( *maybePath ); }
  if ( auto raw = this->rawParams.getGlobalManifestRaw(); raw.has_value() )
    {
      this->initGlobalManifest( resolver::GlobalManifest( *maybePath, *raw ) );
    }

  /* Init manifest. */
  auto path = this->rawParams.getManifestPath();
  this->initManifestPath( path );
  this->initManifest(
    resolver::Manifest( path, this->rawParams.getManifestRaw() ) );

  /* Init lockfile . */
  maybePath = this->rawParams.getLockfilePath();
  if ( maybePath.has_value() ) { this->initLockfilePath( *maybePath ); }
  if ( auto raw = this->rawParams.getLockfileRaw(); raw.has_value() )
    {
      this->initLockfile( resolver::Lockfile( *maybePath, *raw ) );
    }
}


/* -------------------------------------------------------------------------- */

int
SearchCommand::run()
{
  /* Initialize environment. */
  this->initEnvironment();

  pkgdb::PkgQueryArgs args = this->getEnvironment().getCombinedBaseQueryArgs();
  for ( const auto & [name, input] :
        *this->getEnvironment().getPkgDbRegistry() )
    {
      this->rawParams.query.fillPkgQueryArgs( args );
      auto query = pkgdb::PkgQuery( args );
      auto dbRO  = input->getDbReadOnly();
      for ( const auto & row : query.execute( dbRO->db ) )
        {
          std::cout << input->getRowJSON( row ).dump() << std::endl;
        }
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::search


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
