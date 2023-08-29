/* ========================================================================== *
 *
 * @file command.cc
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/core/command.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/write.hh"

/* -------------------------------------------------------------------------- */

namespace flox::command {

/* -------------------------------------------------------------------------- */

VerboseParser::VerboseParser( const std::string & name
                            , const std::string & version
                            )
  : argparse::ArgumentParser( name, version, argparse::default_arguments::help )
{
    this->add_argument( "-q", "--quiet" )
      .help(
        "Decrease the logging verbosity level. May be used up to 3 times."
      ).action(
        [&]( const auto & )
        {
          nix::verbosity =
            ( nix::verbosity <= nix::lvlError )
              ? nix::lvlError : (nix::Verbosity) ( nix::verbosity - 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append();

    this->add_argument( "-v", "--verbose" )
      .help( "Increase the logging verbosity level. May be used up to 4 times." )
      .action(
        [&]( const auto & )
        {
          nix::verbosity =
            ( nix::lvlVomit <= nix::verbosity )
              ? nix::lvlVomit : (nix::Verbosity) ( nix::verbosity + 1 );
        }
      ).default_value( false ).implicit_value( true )
      .append();
}


/* -------------------------------------------------------------------------- */

  void
FloxFlakeMixin::parseFloxFlake( const std::string & flakeRef )
{
  {
    nix::FlakeRef ref = flox::parseFlakeRef( flakeRef );
    nix::Activity act(
      * nix::logger
    , nix::lvlInfo
    , nix::actUnknown
    , nix::fmt( "fetching flake '%s'", ref.to_string() )
    );
    this->flake = std::make_shared<flox::FloxFlake>( this->getState(), ref );
  }

  this->flake = this->FloxFlakeParserMixin::parseFloxFlake( flakeRef );

  if ( ! this->flake->lockedFlake.flake.lockedRef.input.hasAllInfo()
     )
    {
      if ( nix::lvlWarn <= nix::verbosity )
        {
          nix::logger->warn(
            "flake-reference is unlocked/dirty - "
            "resulting DB may not be cached."
          );
        }
    }
}


  argparse::Argument &
FloxFlakeMixin::addFlakeRefArg( argparse::ArgumentParser & parser )
{
  return parser.add_argument( "flake-ref" )
               .help(
                 "flake-ref URI string or JSON attrs ( preferably locked )"
               )
               .required()
               .metavar( "FLAKE-REF" )
               .action( [&]( const std::string & ref )
                        {
                          this->parseFloxFlake( ref );
                        }
                      );
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
                        {
                          this->attrPath.emplace_back( path );
                        }
                      );
}


  void
AttrPathMixin::postProcessArgs()
{
  if ( this->attrPath.empty() ) { this->attrPath.push_back( "packages" ); }
  if ( this->attrPath.size() < 2 )
    {
      this->attrPath.push_back(
        nix::settings.thisSystem.get()
      );
    }
  if ( ( this->attrPath.size() < 3 ) &&
       ( this->attrPath[0] == "catalog" )
     )
    {
      this->attrPath.push_back( "stable" );
    }
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::command' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
