/* ========================================================================== *
 *
 * @file resolver/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nlohmann/json.hpp>

#include "flox/resolver/command.hh"
#include "flox/resolver/resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

argparse::Argument &
ResolveCommand::addResolveParamArgs( argparse::ArgumentParser &parser )
{
  return parser.add_argument( "parameters" )
    .help( "resolution paramaters as inline JSON or a path to a file" )
    .required()
    .metavar( "PARAMS" )
    .action(
      [&]( const std::string &params )
      {
        nlohmann::json paramsJSON = parseOrReadJSONObject( params );
        paramsJSON.get_to( this->params );
      } );
}


/* -------------------------------------------------------------------------- */

ResolveCommand::ResolveCommand() : parser( "resolve" )
{
  this->parser.add_description(
    "Resolve a descriptor in a set of flakes and emit a list "
    "satisfactory packages" );
  this->addResolveParamArgs( this->parser );
}


/* -------------------------------------------------------------------------- */

ResolverState
ResolveCommand::getResolverState() const
{
  return { this->params.registry,
           dynamic_cast<const pkgdb::QueryPreferences &>( this->params ) };
}


/* -------------------------------------------------------------------------- */

int
ResolveCommand::run()
{
  auto state = this->getResolverState();
  auto rsl   = resolve( state, this->getQuery() );
  std::cout << nlohmann::json( rsl ).dump() << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

/* Lock Subcommand */

LockCommand::LockCommand() : parser( "lock" )
{
  this->parser.add_description( "Lock a manifest" );
  this->addManifestFileArg( this->parser );
}


/* -------------------------------------------------------------------------- */

int
LockCommand::run()
{
  auto lockedRegistry = this->getLockedRegistry();

  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::optional<Resolved>>>
    lockedDescriptors;

  for ( const auto &[iid, desc] : this->getDescriptors() )
    {
      lockedDescriptors.emplace( iid, this->lockDescriptor( iid, desc ) );
    }

  // TODO: to_json( ManifestRaw )
  nlohmann::json lockfile
    = { { "manifest", readAndCoerceJSON( this->getManifestPath() ) },
        { "registry", std::move( lockedRegistry ) },
        { "packages", std::move( lockedDescriptors ) },
        { "lockfileVersion", 0 } };

  /* Print that bad boii */
  std::cout << lockfile.dump() << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
