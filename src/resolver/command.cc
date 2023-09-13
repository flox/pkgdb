/* ========================================================================== *
 *
 * @file resolver/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/command.hh"
#include "flox/resolver/resolve.hh"


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

   ResolverState
 ResolveCommand::getResolverState() const
 {
   return ResolverState(
     this->params.registry
   , dynamic_cast<const pkgdb::QueryPreferences &>( this->params )
   );
 }



/* -------------------------------------------------------------------------- */

  int
ResolveCommand::run()
{
  this->initRegistry();
  this->scrapeIfNeeded();
  assert( this->registry != nullptr );
  for ( const auto & resolved :
          resolve( this->getResolverState(), this->getQuery() )
      )
    {
      std::cout << resolved << std::endl;
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
