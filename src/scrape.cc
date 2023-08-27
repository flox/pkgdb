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

namespace flox {
  namespace pkgdb {

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

  /* If `--force' was given, clear the `done' fields for the prefix and its
   * descendants to force them to re-evaluate. */
  {
    row_id prefixId = this->db->addOrGetAttrSetId( this->attrPath );
    sqlite3pp::command cmd( this->db->db, R"SQL(
      UPDATE AttrSets SET done = FALSE WHERE id in (
        WITH RECURSIVE Tree AS (
          SELECT id, parent, 0 as depth FROM AttrSets
          WHERE ( id = :root )
          UNION ALL SELECT O.id, O.parent, ( Parent.depth + 1 ) AS depth
          FROM AttrSets O
          JOIN Tree AS Parent ON ( Parent.id = O.parent )
        ) SELECT C.id FROM Tree AS C
        JOIN AttrSets AS Parent ON ( C.parent = Parent.id )
      )
    )SQL" );
    cmd.bind( ":root", (long long) prefixId );
    cmd.execute();
  }

  /* If we haven't processed this prefix before or `--force' was given, open
   * the eval cache and start scraping. */
  if ( ! this->db->completedAttrSet( this->attrPath ) )
    {
      flox::pkgdb::Todos todo;
      if ( flox::MaybeCursor root =
             this->flake->maybeOpenCursor( this->attrPath );
           root != nullptr
         )
        {
          todo.push(
            std::make_pair( this->attrPath, (flox::Cursor) root )
          );
        }

      /* Start a transaction */
      sqlite3pp::transaction txn( this->db->db );
      try
        {
          while ( ! todo.empty() )
            {
              auto & [prefix, cursor] = todo.front();
              this->db->scrape(
                this->state->symbols
              , prefix
              , cursor
              , todo
              );
              todo.pop();
            }

          /* Mark the prefix and its descendants as "done" */
          {
            row_id prefixId = this->db->getAttrSetId( this->attrPath );
            sqlite3pp::command cmd( this->db->db, R"SQL(
              UPDATE AttrSets SET done = TRUE WHERE id in (
                WITH RECURSIVE Tree AS (
                  SELECT id, parent, 0 as depth FROM AttrSets
                  WHERE ( id = :root )
                  UNION ALL SELECT O.id, O.parent, ( Parent.depth + 1 ) AS depth
                  FROM AttrSets O
                  JOIN Tree AS Parent ON ( Parent.id = O.parent )
                ) SELECT C.id FROM Tree AS C
                JOIN AttrSets AS Parent ON ( C.parent = Parent.id )
              )
            )SQL" );
            cmd.bind( ":root", (long long) prefixId );
            cmd.execute();
          }
        }
      catch( const nix::EvalError & )
        {
          txn.rollback();
          throw;
        }
      txn.commit();

    }

  /* Print path to database. */
  std::cout << ( (std::string) * this->dbPath ) << std::endl;
  return EXIT_SUCCESS;  /* GG, GG */
}


/* -------------------------------------------------------------------------- */

  }  /* End namespaces `flox::pkgdb' */
}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
