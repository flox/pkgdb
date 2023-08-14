/* ========================================================================== *
 *
 * @file flox/flox-flake.hh
 *
 * @brief Defines a convenience wrapper that provides various operations on
 *        a `flake`.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <memory>
#include <nix/eval.hh>
#include <nix/eval-inline.hh>
#include <nix/flake/flake.hh>

#include "flox/types.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * Default flags used when locking flakes.
 * - Disable `updateLockFile` and read existing lockfiles directly.
 * - Disable `writeLockFile` to avoid writing generated lockfiles to the
 *   filesystem; this will only occur if there is no existing lockfile.
 */
static const nix::flake::LockFlags defaultLockFlags = {
  .recreateLockFile      = false         /* default */
, .updateLockFile        = false
, .writeLockFile         = false
, .useRegistries         = std::nullopt  /* default */
, .applyNixConfig        = false         /* default */
, .allowUnlocked         = true          /* default */
, .commitLockFile        = false         /* default */
, .referenceLockFilePath = std::nullopt  /* default */
, .outputLockFilePath    = std::nullopt  /* default */
, .inputOverrides        = {}            /* default */
, .inputUpdates          = {}            /* default */
};


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

  private:
    /**
     * A handle for a cached `nix` evaluator associated with @a this flake.
     * This is opened lazily by @a openEvalCache and remains open until @a this
     * object is destroyed.
     */
    std::shared_ptr<nix::eval_cache::EvalCache> _cache;

  public:
          nix::ref<nix::EvalState> state;
    const nix::flake::LockedFlake  lockedFlake;

    FloxFlake(       nix::ref<nix::EvalState>   state
             , const nix::FlakeRef            & ref
             )
      : state( state )
      , lockedFlake( nix::flake::lockFlake(
          * state
        , ref
        , defaultLockFlags
        ) )
    {}

    FloxFlake( nix::ref<nix::EvalState> state, std::string_view ref )
      : FloxFlake( state, flox::parseFlakeRef( ref ) )
    {}

    /**
     * Open a `nix` evaluator ( with an eval cache when possible ) with the
     * evaluated `flake` and its outputs in global scope.
     * @return A `nix` evaluator, potentially with caching.
     */
      nix::ref<nix::eval_cache::EvalCache>
    openEvalCache()
    {
      if ( this->_cache == nullptr )
        {
          auto fingerprint = this->lockedFlake.getFingerprint();
          this->_cache = std::make_shared<nix::eval_cache::EvalCache>(
            ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
            ? std::optional { std::cref( fingerprint ) }
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

    /**
     * Try to open a `nix` evaluator cursor at a given path.
     * If there is no such attribute this routine will return `nullptr`.
     * @param path The attribute path try opening.
     * @return `nullptr` iff there is no such path, otherwise a
     *         @a nix::eval_cache::AttrCursor at @a path.
     */
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

    /**
     * Open a `nix` evaluator cursor at a given path.
     * If there is no such attribute this routine will throw an error.
     * @param path The attribute path to open.
     * @return A @a nix::eval_cache::AttrCursor at @a path.
     */
      Cursor
    openCursor( const std::vector<nix::Symbol> & path )
    {
      Cursor cur = this->openEvalCache()->getRoot();
      for ( const nix::Symbol & p : path ) { cur = cur->getAttr( p ); }
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
