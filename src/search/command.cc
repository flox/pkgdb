/* ========================================================================== *
 *
 * @file search/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <argparse/argparse.hpp>
#include <nix/ref.hh>
#include <nlohmann/json.hpp>

#include "flox/core/command.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/input.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/pkgdb/read.hh"
#include "flox/registry.hh"
#include "flox/resolver/environment.hh"
#include "flox/resolver/lockfile.hh"
#include "flox/resolver/manifest.hh"
#include "flox/search/command.hh"
#include "flox/search/params.hh"


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
        searchParamsRaw.get_to( this->params );
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
  auto maybePath = this->params.getGlobalManifestPath();
  if ( maybePath.has_value() ) { this->initGlobalManifestPath( *maybePath ); }
  if ( auto raw = this->params.getGlobalManifestRaw(); raw.has_value() )
    {
      this->initGlobalManifest( resolver::GlobalManifest( *maybePath, *raw ) );
    }

  /* Init manifest. */
  auto path = this->params.getManifestPath();
  this->initManifestPath( path );
  this->initManifest(
    resolver::Manifest( path, this->params.getManifestRaw() ) );

  /* Init lockfile . */
  maybePath = this->params.getLockfilePath();
  if ( maybePath.has_value() ) { this->initLockfilePath( *maybePath ); }
  if ( auto raw = this->params.getLockfileRaw(); raw.has_value() )
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
      this->params.query.fillPkgQueryArgs( args );
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
