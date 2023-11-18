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


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/* Manifest Subcommand */

LockCommand::LockCommand() : parser( "lock" )
{
  this->parser.add_description( "Lock a manifest" );
  this->addGlobalManifestFileOption( this->parser );
  this->addManifestFileArg( this->parser );
  this->addLockfileOption( this->parser );
  this->addGARegistryOption( this->parser );
}


/* -------------------------------------------------------------------------- */

int
LockCommand::run()
{
  // TODO: `RegistryRaw' should drop empty fields.
  nlohmann::json lockfile
    = this->getEnvironment().createLockfile().getLockfileRaw();
  /* Print that bad boii */
  std::cout << lockfile.dump() << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

DiffCommand::DiffCommand() : parser( "diff" )
{
  this->parser.add_description( "Diff two manifest files" );

  this->parser.add_argument( "old-manifest" )
    .help( "path to old manifest file" )
    .required()
    .metavar( "OLD-MANIFEST" )
    .action( [&]( const std::string & path )
             { this->oldManifestPath = path; } );

  this->parser.add_argument( "new-manifest" )
    .help( "path to new manifest file" )
    .required()
    .metavar( "NEW-MANIFEST" )
    .action( [&]( const std::string & path ) { this->manifestPath = path; } );
}


/* -------------------------------------------------------------------------- */

const ManifestRaw &
DiffCommand::getManifestRaw()
{
  if ( ! this->manifestRaw.has_value() )
    {
      if ( ! this->manifestPath.has_value() )
        {
          throw FloxException( "you must provide a path to a manifest file." );
        }
      if ( ! std::filesystem::exists( *this->manifestPath ) )
        {
          throw InvalidManifestFileException( "manifest file `"
                                              + this->manifestPath->string()
                                              + "'does not exist." );
        }
      this->manifestRaw = readAndCoerceJSON( *this->manifestPath );
    }
  return *this->manifestRaw;
}


/* -------------------------------------------------------------------------- */

const ManifestRaw &
DiffCommand::getOldManifestRaw()
{
  if ( ! this->oldManifestRaw.has_value() )
    {
      if ( ! this->oldManifestPath.has_value() )
        {
          throw FloxException(
            "you must provide a path to an old manifest file." );
        }
      if ( ! std::filesystem::exists( *this->oldManifestPath ) )
        {
          throw InvalidManifestFileException( "old manifest file `"
                                              + this->oldManifestPath->string()
                                              + "'does not exist." );
        }
      this->oldManifestRaw = readAndCoerceJSON( *this->oldManifestPath );
    }
  return *this->oldManifestRaw;
}


/* -------------------------------------------------------------------------- */

int
DiffCommand::run()
{
  auto diff = this->getOldManifestRaw().diff( this->getManifestRaw() );
  std::cout << diff.dump() << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

ManifestCommand::ManifestCommand() : parser( "manifest" ), cmdLock(), cmdDiff()
{
  this->parser.add_description( "Manifest subcommands" );
  this->parser.add_subparser( this->cmdLock.getParser() );
  this->parser.add_subparser( this->cmdDiff.getParser() );
}


/* -------------------------------------------------------------------------- */

int
ManifestCommand::run()
{
  if ( this->parser.is_subcommand_used( "lock" ) )
    {
      return this->cmdLock.run();
    }
  if ( this->parser.is_subcommand_used( "diff" ) )
    {
      return this->cmdDiff.run();
    }
  std::cerr << this->parser << std::endl;
  throw flox::FloxException( "You must provide a valid `manifest' subcommand" );
  return EXIT_FAILURE;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
