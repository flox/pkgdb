/* ========================================================================== *
 *
 * @file search/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <vector>
#include <functional>
#include <fstream>

#include <nlohmann/json.hpp>

#include "flox/core/util.hh"
#include "flox/flox-flake.hh"
#include "flox/pkgdb/write.hh"
#include "flox/search/command.hh"
#include "flox/pkgdb/command.hh"


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
               .action( [&]( const std::string & query )
                        {
                          pkgdb::PkgQueryArgs args;
                          nlohmann::json::parse( query ).get_to( args );
                          this->query = pkgdb::PkgQuery( args );
                        }
                      );
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
               .action( [&]( const std::string & params )
                        {
                          nlohmann::json paramsJSON =
                            parseOrReadJSONObject( params );
                          paramsJSON.get_to( this->params );
                        }
                      );
}


/* -------------------------------------------------------------------------- */

SearchCommand::SearchCommand() : parser( "search" )
{
  this->parser.add_description(
    "Search a set of flakes and emit a list satisfactory packages"
  );
  this->addSearchParamArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

  int
SearchCommand::run()
{
  this->initRegistry();
  this->scrapeIfNeeded();
  assert( this->registry != nullptr );
  pkgdb::PkgQueryArgs args;
  for ( const auto & [name, input] : * this->registry )
    {
      this->query =
        pkgdb::PkgQuery( this->params.fillPkgQueryArgs( name, args ) );
      for ( const auto & row : this->queryDb( * input->getDbReadOnly() ) )
        {
          this->showRow( * input, row );
        }
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
