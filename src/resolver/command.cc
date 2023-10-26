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

  // TODO: Handle multiple inputs
  auto [name, input] = * this->getPkgDbRegistry()->begin();
  auto dbRO  = this->getPkgDbRegistry()->begin()->second->getDbReadOnly();
  nlohmann::json install = nlohmann::json::object();
  for ( const auto & [id, desc] : this->getUnlockedManifest().getManifestRaw().install )
    {
      auto args = this->getBaseQueryArgs();
      desc.fillPkgQueryArgs( args );
      auto query = flox::pkgdb::PkgQuery( args );
      auto rows  = query.execute( dbRO->db );
      if ( rows.empty() )
        {
          if ( desc.optional )
            {
              install.emplace( id, nullptr );
              continue;
            }
          throw FloxException( "No packages found for `install." + id + "'!" );
        }
      else
        {
          auto info = dbRO->getPackage( rows.front() );
          install.emplace( id, nlohmann::json {
            { "input", name }
          , { "abs-path", info.at( "absPath" ) }
          } );
        }
    }

  // TODO: to_json( ManifestRaw )
  nlohmann::json lockfile
    = { { "manifest", readAndCoerceJSON( this->getManifestPath() ) },
        { "registry", std::move( lockedRegistry ) },
        { "install", std::move( install ) },
        { "lockfileVersion", 0 } };
  for ( const auto &[name, input] :
        lockfile.at( "registry" ).at( "inputs" ).items() )
    {
      /* Delete a few metadata fields that we don't really care about. */
      auto type = input.at( "from" ).at( "type" );
      input.at( "from" ).erase( "lastModified" );
      /* Only keep `narHash' for tarballs and files. */
      if ( ! ( ( type == "tarball" ) || ( type == "file" ) ) )
        {
          input.at( "from" ).erase( "narHash" );
        }

      /* Drop `stabilities' for non-catalogs. */
      bool isCatalog = false;
      for ( const auto &subtree : input.at( "subtrees" ) )
        {
          if ( subtree == "catalog" ) { isCatalog = true; }
        }
      if ( ! isCatalog ) { input.erase( "stabilities" ); }
    }

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
