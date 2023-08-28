/* ========================================================================== *
 *
 * @file flox/nix-state.cc
 *
 * @brief Manages a `nix` runtime state blob with associated helpers.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/store-api.hh>
#include <nix/flake/flake.hh>

#include "flox/core/nix-state.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Perform one time `nix` runtime setup.
 * You may safely call this function multiple times, after the first invocation
 * it is effectively a no-op.
 */
  void
initNix()
{
  static bool didNixInit = false;
  if ( didNixInit ) { return; }

  nix::setStackSize( ( ( (size_t) 64 ) * 1024 ) * 1024 );  // NOLINT
  nix::initNix();
  nix::initGC();
  /* Suppress benign warnings about `nix.conf'. */
  nix::Verbosity oldVerbosity = nix::verbosity;
  nix::verbosity = nix::lvlError;
  nix::initPlugins();
  /* Restore verbosity to `nix' global setting */
  nix::verbosity = oldVerbosity;

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
