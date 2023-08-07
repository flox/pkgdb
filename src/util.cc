/* ========================================================================== *
 *
 * @file flox/util.cc
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

static bool didNixInit = false;

/**
 * Perform one time `nix` runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
  void
initNix( nix::Verbosity verbosity )
{
  if ( didNixInit ) { return; }

  /* Assign verbosity to `nix' global setting */
  nix::verbosity = verbosity;
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
  /* Suppress benign warnings about `nix.conf'. */
  nix::verbosity = nix::lvlError;
  nix::initPlugins();
  /* Restore verbosity to `nix' global setting */
  nix::verbosity = verbosity;

  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.setDefault( true );

  didNixInit = true;
}


/* -------------------------------------------------------------------------- */

}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
