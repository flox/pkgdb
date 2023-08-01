/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <memory>
#include <nix/eval.hh>
#include <nix/eval-inline.hh>
#include <nix/flake/flake.hh>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * A convenience wrapper that provides various operations on a `flake`.
 *
 * Notably this class is responsible for a `nix` `EvalState` and an
 * `EvalCache` database associated with a `flake`.
 *
 * It is recommended that only one `FloxFlake` be created for a unique `flake`
 * to avoid synchronization slowdowns with its databases.
 */
class FloxFlake : public std::enable_shared_from_this<FloxFlake> {
  public:
    using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
    using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;

  private:
    std::shared_ptr<nix::eval_cache::EvalCache> _cache;

  public:
    const nix::flake::LockedFlake  lockedFlake;
          nix::ref<nix::EvalState> state;

    FloxFlake(       nix::ref<nix::EvalState>   state
             , const nix::FlakeRef            & ref
             )
      : state( state )
      , lockedFlake( nix::flake::lockFlake(
          * this->state
        , ref
        , (nix::flake::LockFlags) {
            .updateLockFile = false, .writeLockFile = false
          }
        ) )
    {}

      nix::ref<nix::eval_cache::EvalCache>
    openEvalCache()
    {
      if ( this->_cache == nullptr )
        {
          this->_cache = std::make_shared<nix::eval_cache::EvalCache>(
            ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
            ? std::optional { std::cref( this->lockedFlake->getFingerprint() ) }
            : std::nullopt
          , * this->state
          , [&]()
            {
              nix::Value * vFlake = this->state->allocValue();
              nix::flake::callFlake(
                * this->state
              , this->lockedFlake
              , * vFlake
              );
              this->state->forceAttrs(
                * vFlake, nix::noPos, "while parsing cached flake data"
              );
              nix::Attr * aOutputs = vFlake->attrs->get(
                this->state->symbols.create( "outputs" )
              );
              assert( aOutputs != nullptr );
              return aOutputs->value;
            }
          );
        }
      return (nix::ref<nix::eval_cache::EvalCache>) this->_cache;
    }

      Cursor
    openCursor( const std::vector<nix::Symbol> & path )
    {
      Cursor cur = this->openEvalCache()->getRoot();
      for ( const nix::Symbol & p : path ) { cur = cur->getAttr( p ); }
      return cur;
    }

      MaybeCursor
    maybeOpenCursor( const std::vector<nix::Symbol> & path )
    {
      MaybeCursor cur = this->openEvalCache()->getRoot();
      for ( const nix::Symbol & p : path )
        {
          cur = cur->maybeGetAttr( p );
          if ( cur == nullptr ) { break; }
        }
      return cur;
    }


};  /* End class `FloxFlake' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
