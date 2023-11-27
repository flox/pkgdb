/* ========================================================================== *
 *
 * @file flox/nix-state.cc
 *
 * @brief Manages a `nix` runtime state blob with associated helpers.
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>

#include <nix/config.hh>
#include <nix/error.hh>
#include <nix/eval.hh>
#include <nix/globals.hh>
#include <nix/logging.hh>
#include <nix/shared.hh>
#include <nix/util.hh>
#include <nix/value-to-json.hh>

#include "flox/core/nix-state.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

void
initNix()
{
  static bool didNixInit = false;
  if ( didNixInit ) { return; }

  // NOLINTNEXTLINE
  nix::setStackSize( ( std::size_t( 64 ) * 1024 ) * 1024 );
  nix::initNix();
  nix::initGC();
  /* Suppress benign warnings about `nix.conf'. */
  nix::Verbosity oldVerbosity = nix::verbosity;
  nix::verbosity              = nix::lvlError;
  nix::initPlugins();
  /* Restore verbosity to `nix' global setting */
  nix::verbosity = oldVerbosity;

  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.assign( true );
  nix::experimentalFeatureSettings.experimentalFeatures.assign(
    std::set( { nix::Xp::Flakes } ) );

  /* Use custom logger */
  bool printBuildLogs = nix::logger->isVerbose();
  if ( nix::logger != nullptr ) { delete nix::logger; }
  nix::logger = makeFilteredLogger( printBuildLogs );

  didNixInit = true;
}


/* -------------------------------------------------------------------------- */

nix::FlakeRef
valueToFlakeRef( nix::EvalState &    state,
                 nix::Value &        value,
                 const nix::PosIdx   pos,
                 const std::string & errorMsg )
{
  nix::NixStringContext context;
  if ( value.isThunk() && value.isTrivial() )
    {
      state.forceValue( value, pos );
    }
  auto type = value.type();
  if ( type == nix::nAttrs )
    {
      state.forceAttrs( value, pos, errorMsg );
      return nix::FlakeRef::fromAttrs( nix::fetchers::jsonToAttrs(
        nix::printValueAsJSON( state, true, value, pos, context, false ) ) );
    }
  else if ( type == nix::nString )
    {
      state.forceStringNoCtx( value, pos, errorMsg );
      return nix::parseFlakeRef( std::string( value.str() ) );
    }
  else
    {
      state
        .error( "flake reference was expected to be a set or a string, but "
                "got '%s'",
                nix::showType( type ) )
        .debugThrow<nix::EvalError>();
    }
}


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
