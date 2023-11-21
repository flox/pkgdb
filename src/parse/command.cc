/* ========================================================================== *
 *
 * @file parse/command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/parse/command.hh"


/* -------------------------------------------------------------------------- */

namespace flox::parse {

/* -------------------------------------------------------------------------- */

DescriptorCommand::DescriptorCommand() :parser( "descriptor" )
{
  this->parser.add_description( "Parse a package descriptor" );
  this->parser.add_argument( "descriptor" )
    .help( "a package descriptor to parse" )
    .metavar( "DESCRIPTOR" )
    .action( [&]( const std::string & desc )
             {
               // TODO: ManifestDescriptor from string
               this->descriptor = resolver::ManifestDescriptor(
                 nlohmann::json( desc ).get<resolver::ManifestDescriptorRaw>()
               );
             } );
}


/* -------------------------------------------------------------------------- */

int
DescriptorCommand::run()
{
  pkgdb::PkgQueryArgs args;
  this->descriptor.fillPkgQueryArgs( args );
  std::cout << nlohmann::json( args ).dump( 2 ) << std::endl;
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

ParseCommand::ParseCommand() : parser( "parse" )
{
  this->parser.add_description( "Parse various constructs" );
  this->parser.add_subparser( this->cmdDescriptor.getParser() );
}


/* -------------------------------------------------------------------------- */

int
ParseCommand::run()
{
  if ( this->parser.is_subcommand_used( "descriptor" ) )
    {
      return this->cmdDescriptor.run();
    }
  std::cerr << this->parser << std::endl;
  throw flox::FloxException( "You must provide a valid `parse' subcommand" );
  return EXIT_FAILURE;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::parse


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
