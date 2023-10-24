/* ========================================================================== *
 *
 * @file flox-flake.cc
 *
 * @brief Defines a convenience wrapper that provides various operations on
 *        a `flake`.
 *
 *
 * -------------------------------------------------------------------------- */

/* Provides inline definitions of `allocValue', and `forceAttrs'. */
#include <nix/eval-inline.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/util.hh"
#include "flox/flox-flake.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

FloxFlake::FloxFlake( const nix::ref<nix::EvalState> &state,
                      const nix::FlakeRef            &ref )
try : state( state ),
  lockedFlake( nix::flake::lockFlake( *this->state, ref, defaultLockFlags ) )
  {}
catch ( const std::exception &err )
  {

    throw LockFlakeException(
      "failed to lock flake \"" + ref.to_string() + "\"",
      nix::filterANSIEscapes( err.what(), true ).c_str() );
  }
catch ( ... )
  {

    throw LockFlakeException( "failed to lock flake \"" + ref.to_string()
                              + "\"" );
  }


/* -------------------------------------------------------------------------- */

nix::ref<nix::eval_cache::EvalCache>
FloxFlake::openEvalCache()
{
  if ( this->_cache == nullptr )
    {
      auto fingerprint = this->lockedFlake.getFingerprint();
      this->_cache     = std::make_shared<nix::eval_cache::EvalCache>(
        ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
              ? std::optional { std::cref( fingerprint ) }
              : std::nullopt,
        *this->state,
        [&]()
        {
          nix::Value *vFlake = this->state->allocValue();
          nix::flake::callFlake( *this->state, this->lockedFlake, *vFlake );
          this->state->forceAttrs( *vFlake,
                                   nix::noPos,
                                   "while parsing cached flake data" );
          nix::Attr *aOutputs
            = vFlake->attrs->get( this->state->symbols.create( "outputs" ) );
          assert( aOutputs != nullptr );
          return aOutputs->value;
        } );
    }
  return static_cast<nix::ref<nix::eval_cache::EvalCache>>( this->_cache );
}


/* -------------------------------------------------------------------------- */

MaybeCursor
FloxFlake::maybeOpenCursor( const AttrPath &path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const auto &part : path )
    {
      cur = cur->maybeGetAttr( part );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}


/* -------------------------------------------------------------------------- */

Cursor
FloxFlake::openCursor( const AttrPath &path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const auto &part : path ) { cur = cur->getAttr( part ); }
  return cur;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
