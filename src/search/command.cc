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
#include "flox/pkgdb.hh"
#include "flox/search/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces used to search for packages in flakes. */
  namespace search {

/* -------------------------------------------------------------------------- */

  void
InputsMixin::parseInputs( const std::string & jsonOrFile )
{
  nlohmann::json inputsJSON = parseOrReadJSONObject( jsonOrFile );

  for ( const auto & [name, flakeRef] :  inputsJSON.items() )
    {
      nix::FlakeRef parsed = flox::parseFlakeRef( flakeRef.get<std::string>() );
      this->inputs.emplace_back( std::make_pair(
        name
      , Input {
          FloxFlake( (nix::ref<nix::EvalState>) this->state, parsed )
        , nullptr
        }
      ) );
    }
}


/* -------------------------------------------------------------------------- */

  void
InputsMixin::openDatabases()
{
  for ( auto & [name, input] : this->inputs )
    {
      std::filesystem::path dbPath = pkgdb::genPkgDbName(
        input.flake.lockedFlake.getFingerprint()
      );

      /* Initialize DB if none exists. */
      if ( ! std::filesystem::exists( dbPath ) )
        {
          std::filesystem::create_directories( dbPath.parent_path() );
          pkgdb::PkgDb( input.flake.lockedFlake, (std::string) dbPath );
        }

      /* Open a read-only copy. */
      input.db = std::make_unique<pkgdb::PkgDbReadOnly>( (std::string) dbPath );
    }
}


/* -------------------------------------------------------------------------- */

  argparse::Argument &
InputsMixin::addInputsArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "inputs" )
               .help( "flakes to be searched" )
               .required()
               .metavar( "INPUTS" )
               .action( [&]( const std::string & inputs )
                        {
                          this->parseInputs( inputs );
                        }
                      );
}


/* ========================================================================== */

  argparse::Argument &
PkgQueryMixin::addQueryArgs( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "query" )
               .help( "query parameters" )
               .required()
               .metavar( "QUERY" )
               .action( [&]( const std::string & query )
                        {
                          nlohmann::json jqry = nlohmann::json::parse( query );
                          pkgdb::PkgQueryArgs args;
                          pkgdb::from_json( jqry, args );
                          this->query = pkgdb::PkgQuery( args );
                        }
                      );
}


/* -------------------------------------------------------------------------- */

  void
PkgQueryMixin::initQuery()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  std::vector<pkgdb::row_id>
PkgQueryMixin::queryDb( pkgdb::PkgDbReadOnly & db )
{
  return this->query.execute( db.db );
}


/* ========================================================================== */

//SearchCommand::SearchCommand()
//{
//  // TODO
//}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::scrapeIfNeeded()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::postProcessArgs()
{
  // TODO
}


/* -------------------------------------------------------------------------- */

  int
SearchCommand::run()
{
  // TODO
  return EXIT_FAILURE;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::search' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
