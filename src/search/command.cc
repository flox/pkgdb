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

  void
SearchParamsMixin::setInput( const std::string & name )
{
  pkgdb::PkgQueryArgs args;
  this->query = pkgdb::PkgQuery( this->params.fillPkgQueryArgs( name, args ) );
}


/* -------------------------------------------------------------------------- */

  argparse::Argument &
SearchParamsMixin::addSearchParamArgs( argparse::ArgumentParser & parser )
{
  auto parse = [&]( const std::string & params )
    {
      nlohmann::json paramsJSON = parseOrReadJSONObject( params );
      paramsJSON.get_to( this->params );
      for ( const auto & _name : this->params.registry.getOrder() )
        {
          const std::string & name = _name.get();
          this->inputs.emplace_back( std::make_pair(
            name
          , InputsMixin::Input {
              std::make_shared<FloxFlake>(
                this->getState()
              , * this->params.registry.inputs.at( name ).from
              )
            , nullptr
            }
          ) );
        }
    };
  return parser.add_argument( "parameters" )
               .help( "search paramaters as inline JSON or a path to a file" )
               .required()
               .metavar( "PARAMS" )
               .action( std::move( parse ) );
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

// FIXME: this needs to use `setInput' or something, you aren't going to scrape
// the correct subtrees or stabilities here.
  void
SearchCommand::scrapeIfNeeded()
{
  this->openDatabases();
  std::vector<subtree_type> subtrees =
    this->params.registry.defaults.subtrees.value_or(
      std::vector<subtree_type> { ST_PACKAGES, ST_LEGACY, ST_CATALOG }
    );
  std::vector<std::string> stabilities =
    this->params.registry.defaults.stabilities.value_or(
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
      for ( const auto & system : this->params.systems )
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

  void
SearchCommand::showRow(
        std::string_view     inputName
, const InputsMixin::Input & input
,       pkgdb::row_id        row
)
{
  nlohmann::json rsl = input.db->getPackage( row );
  rsl.emplace( "input", inputName );
  rsl.emplace( "path",  input.db->getPackagePath( row ) );
  std::cout << rsl.dump() << std::endl;
}


/* -------------------------------------------------------------------------- */

  int
SearchCommand::run()
{
  this->postProcessArgs();
  for ( const auto & [name, input] : this->inputs )
    {
      this->setInput( name );
      for ( const auto & row : this->queryDb( * input.db ) )
        {
          this->showRow( name, input, row );
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
