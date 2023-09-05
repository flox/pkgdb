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

/** Interfaces used to search for packages in flakes. */
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


/* ========================================================================== */

  argparse::Argument &
SearchParamsMixin::addSearchParamArgs( argparse::ArgumentParser & parser )
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


/* ========================================================================== */

SearchCommand::SearchCommand() : parser( "search" )
{
  this->parser.add_description(
    "Search a set of flakes and emit a list satisfactory DB + "
    "`Packages.id' pairs"
  );
  this->addSearchParamArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::initRegistry()
{
  nix::ref<nix::Store> store = this->getStore();
  this->registry = std::make_shared<Registry<pkgdb::PkgDbInput>>(
    store
  , this->params.registry
  );
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::scrapeIfNeeded()
{
  assert( this->registry != nullptr );
  for ( auto & [name, input] : * this->registry )
    {
      input->scrapeSystems( this->params.systems );
    }
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::postProcessArgs()
{
  static bool didPost = false;
  if ( didPost ) { return; }
  this->initRegistry();
  this->scrapeIfNeeded();
  didPost = true;
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::showRow(
  std::string_view    inputName
, pkgdb::PkgDbInput & input
, pkgdb::row_id       row
)
{
  nlohmann::json rsl = input.getDbReadOnly()->getPackage( row );
  rsl.emplace( "input", inputName );
  rsl.emplace( "path",  input.getDbReadOnly()->getPackagePath( row ) );
  std::cout << rsl.dump() << std::endl;
}


/* -------------------------------------------------------------------------- */

  int
SearchCommand::run()
{
  this->postProcessArgs();
  assert( this->registry != nullptr );
  pkgdb::PkgQueryArgs args;
  for ( const auto & [name, input] : * this->registry )
    {
      this->query =
        pkgdb::PkgQuery( this->params.fillPkgQueryArgs( name, args ) );
      for ( const auto & row : this->queryDb( * input->getDbReadOnly() ) )
        {
          this->showRow( name, * input, row );
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
