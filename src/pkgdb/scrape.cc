/* ========================================================================== *
 *
 * @file scrape.cc
 *
 * @brief Implementation of the `pkgdb scrape` subcommand.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include "flox/pkgdb/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */


/* Scrape Subcommand */

ScrapeCommand::ScrapeCommand() : parser( "scrape" )
{
  this->parser.add_description( "Scrape a flake and emit a SQLite3 DB" );
  this->parser.add_argument( "-f", "--force" )
              .help( "Force re-evaluation of flake" )
              .nargs( 0 )
              .action( [&]( const auto & ) { this->force = true; } );
  this->addDatabasePathOption( this->parser );
  this->addFlakeRefArg( this->parser );
  this->addAttrPathArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

  void
ScrapeCommand::initInput()
{
  nix::ref<nix::Store> store = this->getStore();
  /* Change the database path if `--database' was given. */
  if ( this->dbPath.has_value() )
    {
      this->input = std::make_optional<PkgDbInput>(
        store
      , this->getRegistryInput()
      , * this->dbPath
      , PkgDbInput::db_path_tag()
      );
    }
  else
    {
      this->input = std::make_optional<PkgDbInput>(
        store
      , this->getRegistryInput()
      );
    }
}


/* -------------------------------------------------------------------------- */

  void
ScrapeCommand::postProcessArgs()
{
  static bool didPost = false;
  if ( didPost ) { return; }
  this->fixupAttrPath();
  this->initInput();
  didPost = true;
}


/* -------------------------------------------------------------------------- */

  int
ScrapeCommand::run()
{
  this->postProcessArgs();
  assert( this->input.has_value() );

  /* If `--force' was given, clear the `done' fields for the prefix and its
   * descendants to force them to re-evaluate. */
  if ( this->force )
    {
      this->input->getDbReadWrite()->setPrefixDone( this->attrPath, false );
    }

  /* scrape it up! */
  this->input->scrapePrefix( this->attrPath );

  /* Print path to database. */
  std::cout << ( (std::string) * this->dbPath ) << std::endl;
  return EXIT_SUCCESS;  /* GG, GG */
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
