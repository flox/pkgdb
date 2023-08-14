/* ========================================================================== *
 *
 * @file scrape.cc
 *
 * @brief Implementation of the `pkgdb scrape` subcommand.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include "flox/command.hh"
#include "pkgdb.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace command {

/* -------------------------------------------------------------------------- */


/* Scrape Subcommand */

ScrapeCommand::ScrapeCommand() : flox::NixState(), parser( "scrape" )
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
ScrapeCommand::postProcessArgs()
{
  static bool didPost = false;
  if ( didPost ) { return; }
  this->AttrPathMixin::postProcessArgs();
  this->PkgDbMixin::postProcessArgs();
  didPost = true;
}


/* -------------------------------------------------------------------------- */

  int
ScrapeCommand::run()
{
  this->postProcessArgs();

  /* If we haven't processed this prefix before or `--force' was given, open
   * the eval cache and start scraping. */

  if ( this->force || ( ! this->db->hasPackageSet( this->attrPath ) ) )
    {
      std::vector<nix::Symbol> symbolPath;
      for ( const auto & a : this->attrPath )
        {
          symbolPath.emplace_back( this->state->symbols.create( a ) );
        }

      flox::pkgdb::Todos todo;
      if ( flox::MaybeCursor root =
             this->flake->maybeOpenCursor( symbolPath );
           root != nullptr
         )
        {
          todo.push(
            std::make_pair( this->attrPath, (flox::Cursor) root )
          );
        }

      while ( ! todo.empty() )
        {
          auto & [prefix, cursor] = todo.front();
          flox::pkgdb::scrape(
            * this->db
          , this->state->symbols
          , prefix
          , cursor
          , todo
          );
          todo.pop();
        }
    }

  /* Print path to database. */
  std::cout << ( (std::string) this->dbPath.value() ) << std::endl;
  return EXIT_SUCCESS;  /* GG, GG */
}


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::command' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
