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
 * A convenience wrapper that provides various operations on a `flake'.
 *
 * Notably this class is responsible for a `nix' `EvalState' and an
 * `EvalCache' database associated with a `flake'.
 *
 * It is recommended that only one `FloxFlake' be created for a unique `flake'
 * to avoid synchronization slowdowns with its databases.
 */
class FloxFlake : public std::enable_shared_from_this<FloxFlake> {
  public:
    using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
    using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;

  private:
    nix::ref<nix::EvalState>                    _state;
    std::shared_ptr<nix::eval_cache::EvalCache> _cache;

  public:
    const nix::flake::LockedFlake lockedFlake;

    FloxFlake(       nix::ref<nix::EvalState>   state
             , const nix::FlakeRef            & ref
             );

    nix::ref<nix::eval_cache::EvalCache> openEvalCache();

    /**
     * Like `findAttrAlongPath' but without suggestions.
     * Note that each invocation opens the `EvalCache', so use sparingly.
     */
    Cursor      openCursor(      const std::vector<nix::Symbol> & path );
    MaybeCursor maybeOpenCursor( const std::vector<nix::Symbol> & path );

};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
