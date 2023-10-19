/* ========================================================================== *
 *
 * @file command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <iostream>

#include <nix/eval-cache.hh>
#include <nix/eval.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>

#include "flox/core/command.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/write.hh"

/* -------------------------------------------------------------------------- */

namespace flox::command {

/* -------------------------------------------------------------------------- */

VerboseParser::VerboseParser( const std::string & name,
                              const std::string & version )
  : argparse::ArgumentParser( name, version, argparse::default_arguments::help )
{
  this->add_argument( "-q", "--quiet" )
    .help( "Decrease the logging verbosity level. May be used up to 3 times." )
    .action(
      [&]( const auto & )
      {
        nix::verbosity = ( nix::verbosity <= nix::lvlError )
                           ? nix::lvlError
                           : static_cast<nix::Verbosity>( nix::verbosity - 1 );
      } )
    .default_value( false )
    .implicit_value( true )
    .append();

  this->add_argument( "-v", "--verbose" )
    .help( "Increase the logging verbosity level. May be used up to 4 times." )
    .action(
      [&]( const auto & )
      {
        nix::verbosity = ( nix::lvlVomit <= nix::verbosity )
                           ? nix::lvlVomit
                           : static_cast<nix::Verbosity>( nix::verbosity + 1 );
      } )
    .default_value( false )
    .implicit_value( true )
    .append();
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
InlineInputMixin::addFlakeRefArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "flake-ref" )
    .help( "flake-ref URI string or JSON attrs ( preferably locked )" )
    .required()
    .metavar( "FLAKE-REF" )
    .action( [&]( const std::string & flakeRef )
             { this->parseFlakeRef( flakeRef ); } );
}


argparse::Argument &
InlineInputMixin::addSubtreeArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--subtree" )
    .help( "A subtree name, being one of `packages`, `legacyPackages`, "
           "or `catalog', that should be processed. "
           "May be used multiple times." )
    .required()
    .metavar( "SUBTREE" )
    .action(
      [&]( const std::string & subtree )
      {
        /* Parse the subtree type to an enum. */
        Subtree stype = Subtree::parseSubtree( subtree );
        /* Create or append the `subtrees' list. */
        if ( this->registryInput.subtrees.has_value() )
          {
            if ( auto has = std::find( this->registryInput.subtrees->begin(),
                                       this->registryInput.subtrees->end(),
                                       stype );
                 has == this->registryInput.subtrees->end() )
              {
                this->registryInput.subtrees->emplace_back( stype );
              }
          }
        else
          {
            this->registryInput.subtrees =
              std::make_optional( std::vector<Subtree> { stype } );
          }
      } );
}


argparse::Argument &
InlineInputMixin::addStabilityArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--stability" )
    .help( "A stability name, being one of `stable`, `staging`, "
           "or `unstable', that should be processed. "
           "May be used multiple times." )
    .required()
    .metavar( "STABILITY" )
    .action(
      [&]( const std::string & stability )
      {
        /* Create or append the `stabilities' list. */
        if ( this->registryInput.stabilities.has_value() )
          {
            if ( auto has = std::find( this->registryInput.stabilities->begin(),
                                       this->registryInput.stabilities->end(),
                                       stability );
                 has == this->registryInput.stabilities->end() )
              {
                this->registryInput.stabilities->emplace_back( stability );
              }
          }
        else
          {
            this->registryInput.stabilities = { std::vector<std::string> {
              stability } };
          }
      } );
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
AttrPathMixin::addAttrPathArgs( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "attr-path" )
    .help( "Attribute path to scrape" )
    .metavar( "ATTRS..." )
    .remaining()
    .action( [&]( const std::string & path )
             { this->attrPath.emplace_back( path ); } );
}


void
AttrPathMixin::fixupAttrPath()
{
  if ( this->attrPath.empty() ) { this->attrPath.push_back( "packages" ); }
  if ( this->attrPath.size() < 2 )
    {
      this->attrPath.push_back( nix::settings.thisSystem.get() );
    }
  if ( ( this->attrPath.size() < 3 ) && ( this->attrPath[0] == "catalog" ) )
    {
      this->attrPath.push_back( "stable" );
    }
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
RegistryFileMixin::addRegistryFileArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "--registry-file" )
    .help( "The path to the 'registry.json' file." )
    .required()
    .metavar( "PATH" )
    .action( [&]( const std::string & strPath )
             { this->setRegistryPath( strPath ); } );
}

void
RegistryFileMixin::setRegistryPath( const std::filesystem::path & path )
{
  if ( path.empty() )
    {
      throw FloxException( "provided registry path is empty" );
    }
  this->registryPath = path;
}

const RegistryRaw &
RegistryFileMixin::getRegistryRaw()
{
  if ( this->registryRaw.has_value() ) { return *this->registryRaw; }
  this->loadRegistry();
  return *this->registryRaw;
}

void
RegistryFileMixin::loadRegistry()
{
  if ( ! this->registryPath.has_value() )
    {
      throw FloxException( "You must provide a path to a 'registry.json', "
                           "see the '--registry-file' option." );
    }
  std::ifstream  f( *( this->registryPath ) );
  nlohmann::json json = nlohmann::json::parse( f );
  json.get_to( this->registryRaw );
}

/* -------------------------------------------------------------------------- */

}  // namespace flox::command


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
