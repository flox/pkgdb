/* ========================================================================== *
 *
 * @file pkgdb/primops.cc
 *
 * @brief Extensions to `nix` primitive operations.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/json-to-value.hh>
#include <nix/primops.hh>
#include <nix/value-to-json.hh>
#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
#include "flox/pkgdb/primops.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

void
prim_getFingerprint( nix::EvalState &  state,
                     const nix::PosIdx pos,
                     nix::Value **     args,
                     nix::Value &      value )
{
  nix::NixStringContext context;

  if ( args[0]->isThunk() && args[0]->isTrivial() )
    {
      state.forceValue( *args[0], pos );
    }
  auto          type = args[0]->type();
  RegistryInput input;
  if ( type == nix::nAttrs )
    {
      state.forceAttrs(
        *args[0],
        pos,
        "while processing 'flakeRef' argument to 'builtins.getFingerprint'" );
      input = RegistryInput(
        nlohmann::json { { "from",
                           nix::printValueAsJSON( state,
                                                  true,
                                                  *args[0],
                                                  pos,
                                                  context,
                                                  false ) } } );
    }
  else if ( type == nix::nString )
    {
      state.forceStringNoCtx(
        *args[0],
        pos,
        "while processing 'flakeRef' argument to 'builtins.getFingerprint'" );
      input
        = RegistryInput( nix::parseFlakeRef( std::string( args[0]->str() ) ) );
    }
  else
    {
      state
        .error( "flake reference was expected to be a set or a string, but "
                "got '%s'",
                nix::showType( type ) )
        .debugThrow<nix::EvalError>();
    }

  FloxFlakeInput flake( state.store, input );
  value.mkString(
    flake.getFlake()->lockedFlake.getFingerprint().to_string( nix::Base16,
                                                              false ) );
}


/* -------------------------------------------------------------------------- */

static nix::RegisterPrimOp primop_getFingerprint( { .name  = "__getFingerprint",
                                                    .args  = { "flakeRef" },
                                                    .arity = 0,
                                                    .doc   = R"(
    This hash uniquely identifies a revision of a locked flake.
    Takes a single argument:

    - `flakeRef`: Either an attribute set or string flake-ref.
    )",
                                                    .fun = prim_getFingerprint,
                                                    .experimentalFeature
                                                    = nix::Xp::Flakes } );


/* -------------------------------------------------------------------------- */

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
