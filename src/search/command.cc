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

  void
InputsMixin::parseInputs( const std::string & jsonOrFile )
{
  nlohmann::json inputsJSON = parseOrReadJSONObject( jsonOrFile );

  for ( const auto & [name, flakeRef] :  inputsJSON.items() )
    {
      this->inputs.emplace_back( std::make_pair(
        name
      , Input { this->parseFloxFlakeJSON( flakeRef ), nullptr }
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
        input.flake->lockedFlake.getFingerprint()
      );

      /* Initialize DB if none exists. */
      if ( ! std::filesystem::exists( dbPath ) )
        {
          std::filesystem::create_directories( dbPath.parent_path() );
          pkgdb::PkgDb( input.flake->lockedFlake, (std::string) dbPath );
        }

      /* Open a read-only copy. */
      input.db = std::make_shared<pkgdb::PkgDbReadOnly>( (std::string) dbPath );
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

  std::vector<pkgdb::row_id>
PkgQueryMixin::queryDb( pkgdb::PkgDbReadOnly & pdb ) const
{
  return this->query.execute( pdb.db );
}


/* ========================================================================== */

SearchCommand::SearchCommand() : parser( "search" )
{
  this->parser.add_description(
    "Search a set of flakes and emit a list satisfactory DB + "
    "`Packages.id' pairs"
  );
  this->addInputsArg( this->parser );
  this->addQueryArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::scrapeIfNeeded()
{
  this->openDatabases();
  std::vector<subtree_type> subtrees = this->query.subtrees.value_or(
    std::vector<subtree_type> { ST_PACKAGES, ST_LEGACY, ST_CATALOG }
  );
  std::vector<std::string> stabilities = this->query.stabilities.value_or(
    std::vector<std::string> { "stable" }
  );

  auto maybeScrape =
    [&]( const flox::AttrPath & path, Input & input ) -> void
    {
      if ( input.db->completedAttrSet( path ) ) { return; }
      pkgdb::ScrapeCommand cmd {};
      cmd.store    = (std::shared_ptr<nix::Store>)     this->getStore();
      cmd.state    = (std::shared_ptr<nix::EvalState>) this->getState();
      cmd.dbPath   = { input.db->dbPath };
      cmd.flake    = input.flake;
      cmd.attrPath = path;
      cmd.openPkgDb();
      /* Push `STDOUT' fd. */
      std::ofstream    out( "/dev/null" );
      std::streambuf * coutbuf = std::cout.rdbuf();
      std::cout.rdbuf( out.rdbuf() );
      int rsl = cmd.run();
      std::cout.rdbuf( coutbuf );  /* Restore `STDOUT' */
      if ( rsl != EXIT_SUCCESS )
        {
          throw FloxException( nix::fmt(
            "Encountered error scraping flake `%s' at prefix `%s'"
          , input.flake->lockedFlake.flake.lockedRef.to_string()
          , nix::concatStringsSep( ".", path )
          ) );
        }
    };

  auto scrapePrefix = [&]( const flox::AttrPath & prefix )
    {
      for ( auto & [name, input] : this->inputs )
        {
          maybeScrape( prefix, input );
        }
    };

  for ( const auto & subtree : subtrees )
    {
      flox::AttrPath prefix;
      switch ( subtree )
        {
          case ST_PACKAGES: prefix = { "packages" };         break;
          case ST_LEGACY:   prefix = { "legacyPackages" };   break;
          case ST_CATALOG:  prefix = { "catalog" };          break;
          default: throw FloxException( "Invalid subtree" ); break;
        }
      for ( const auto & system : this->query.systems )
        {
          prefix.emplace_back( system );
          if ( subtree != ST_CATALOG )
            {
              scrapePrefix( prefix );
            }
          else
            {
              for ( const auto & stability : stabilities )
                {
                  prefix.emplace_back( stability );
                  scrapePrefix( prefix );
                }
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

  void
SearchCommand::postProcessArgs()
{
  static bool didPost = false;
  if ( didPost ) { return; }
  this->openDatabases();
  this->scrapeIfNeeded();
  didPost = true;
}


/* -------------------------------------------------------------------------- */

  int
SearchCommand::run()
{
  this->postProcessArgs();
  for ( const auto & [name, input] : this->inputs )
    {
      for ( const auto & row : this->queryDb( * input.db ) )
        {
          std::cout << input.db->dbPath.string() << " " << row << std::endl;
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
