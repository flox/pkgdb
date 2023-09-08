/* ========================================================================== *
 *
 * @file resolver/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

  argparse::Argument &
ResolveCommand::addResolveParamArgs( argparse::ArgumentParser & parser )
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

ResolveCommand::ResolveCommand() : parser( "resolve" )
{
  this->parser.add_description(
    "Resolve a descriptor in a set of flakes and emit a list "
    "satisfactory packages"
  );
  this->addResolveParamArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

  void
ResolveCommand::showRow(
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
ResolveCommand::run()
{
  this->initRegistry();
  this->scrapeIfNeeded();
  assert( this->registry != nullptr );
  pkgdb::PkgQueryArgs args;
  for ( const auto & [name, input] : * this->registry )
    {
      /* We will get `false` if we should skip this input entirely. */
      bool shouldSearch = this->params.fillPkgQueryArgs( name, args );
      if ( ! shouldSearch ) { continue; }
      this->query = pkgdb::PkgQuery( args );
      for ( const auto & row : this->queryDb( * input->getDbReadOnly() ) )
        {
          this->showRow( name, * input, row );
        }
    }
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
