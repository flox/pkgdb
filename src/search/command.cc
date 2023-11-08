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
        nlohmann::json searchParamsRaw = parseOrReadJSONObject( params );
        searchParamsRaw.get_to( this->rawParams );
        // TODO: delete everything after this
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
  // TODO:
  // - In EnvironmentMixin::search:
  //     - Construct PkgQueryArgs from SearchParamsRaw
  //     - Loop over registry entries via PkgDbRegistryMixin
  //     - Construct a PkgQuery from the PkgQueryArgs
  //     - Query the database via PkgQueryMixin::queryDb

  // this->search( this->query );
  // pkgdb::PkgQueryArgs args;
  // for ( const auto & [name, input] : *this->getPkgDbRegistry() )
  //   {
  //     this->params.fillPkgQueryArgs( name, args );
  //     this->query = pkgdb::PkgQuery( args );
  //     for ( const auto & row : this->queryDb( *input->getDbReadOnly() ) )
  //       {
  //         this->showRow( *input, row );
  //       }
  //   }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json & jfrom, SearchParamsRaw & raw )
{
  assertIsJSONObject<ParseSearchQueryException>( jfrom, "search query" );
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "globalManifestRaw" )
        {
          try
            {
              value.get_to( raw.globalManifestRaw );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw ParseSearchQueryException(
                "couldn't interpret search query field 'globalManifestRaw'",
                extract_json_errmsg( e ) );
            }
        }
      else if ( key == "lockfileRaw" )
        {
          try
            {
              value.get_to( raw.lockfileRaw );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw ParseSearchQueryException(
                "couldn't interpret search query field 'lockfileRaw' ",
                extract_json_errmsg( e ) );
            }
        }
      else if ( key == "query" )
        {

          try
            {
              value.get_to( raw.query );
            }
          catch ( nlohmann::json::exception & e )
            {
              throw ParseSearchQueryException(
                "couldn't interpret search query field 'query'",
                extract_json_errmsg( e ) );
            }
        }
      else
        {
          throw ParseSearchQueryException( "unrecognized field '" + key
                                           + "' in search query" );
        }
    }
}

void
to_json( nlohmann::json & jto, const SearchParamsRaw & raw )
{
  jto = { { "globalManifestRaw", raw.globalManifestRaw },
          { "lockfileRaw", raw.lockfileRaw },
          { "query", raw.query } };
}

/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
SearchParamsRaw::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
{
  this->query.fillPkgQueryArgs( pqa );
  return pqa;
}

}  // namespace flox::search


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
