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
PkgQueryMixin::addQueryArgs( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "query" )
    .help( "query parameters" )
    .required()
    .metavar( "QUERY" )
    .action(
      [&]( const std::string & query )
      {
        pkgdb::PkgQueryArgs args;
        nlohmann::json::parse( query ).get_to( args );
        this->query = pkgdb::PkgQuery( args );
      } );
}


/* -------------------------------------------------------------------------- */

std::vector<pkgdb::row_id>
PkgQueryMixin::queryDb( pkgdb::PkgDbReadOnly & pdb ) const
{
  return this->query.execute( pdb.db );
}


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
        //         [&]( const std::string & inlineJsonOrPath )
        // {
        //   nlohmann::json params = parseOrReadJSONObject( inlineJsonOrPath );
        //   params.get_to( this->rawParams );
        // } );
        nlohmann::json paramsJSON = parseOrReadJSONObject( params );
        paramsJSON.get_to( this->params );
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

int
SearchCommand::run()
{
  pkgdb::PkgQueryArgs args;
  for ( const auto & [name, input] : *this->getPkgDbRegistry() )
    {
      this->params.fillPkgQueryArgs( name, args );
      this->query = pkgdb::PkgQuery( args );
      for ( const auto & row : this->queryDb( *input->getDbReadOnly() ) )
        {
          this->showRow( *input, row );
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
