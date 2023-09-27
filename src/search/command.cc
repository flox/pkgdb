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

#include "flox/core/exceptions.hh"
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
  try
    {
      this->initRegistry();
      this->scrapeIfNeeded();
      assert( this->registry != nullptr );
      pkgdb::PkgQueryArgs args;
      for ( const auto & [name, input] : * this->registry )
        {
          this->params.fillPkgQueryArgs( name, args );
          this->query = pkgdb::PkgQuery( args );
          for ( const auto & row : this->queryDb( * input->getDbReadOnly() ) )
            {
              this->showRow( * input, row );
            }
        }
      return EXIT_SUCCESS;
    }
  catch( const pkgdb::PkgQuery::InvalidArgException & err )
    {
      // TODO: DRY ( see main.cc )
      int exitCode = EC_PKG_QUERY_INVALID_ARG;
      exitCode += static_cast<int>( err.errorCode );
      std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                << exitCode << " }" << std::endl;
      return exitCode;
    }
  catch( const std::exception & err )
    {
      // TODO: DRY ( see main.cc )
      std::cout << "{ \"error\": \"" << err.what() << "\", \"code\": "
                << EC_FAILURE << " }" << std::endl;
      return EC_FAILURE;
    }
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
