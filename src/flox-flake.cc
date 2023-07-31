/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include "flox/types.hh"
#include "flox/util.hh"
#include "flox/flox-flake.hh"
#include "flox/drv-cache.hh"
#include <queue>
#include "flox/flake-package.hh"
#include "flox/cached-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

FloxFlake::FloxFlake(       nix::ref<nix::EvalState>   state
                    ,       std::string_view           id
                    , const FloxFlakeRef             & ref
                    , const Preferences              & prefs
                    , const std::list<std::string>   & systems
                    )
  : _state( state )
  , _flakeRef( nix::FlakeRef::fromAttrs( ref.toAttrs() ) )
  , _systems( systems )
  , _prefsStabilities(
      ( prefs.stabilities.find( std::string( id ) ) != prefs.stabilities.end() )
      ? prefs.stabilities.at( std::string( id ) )
      : defaultCatalogStabilities
    )
  , _prefsPrefixes(
      ( prefs.prefixes.find( std::string( id ) ) != prefs.prefixes.end() )
      ? prefs.prefixes.at( std::string( id ) )
      : defaultSubtrees
    )
{
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<nix::flake::LockedFlake>
FloxFlake::getLockedFlake()
{
  if ( this->lockedFlake == nullptr )
    {
      bool oldPurity = nix::evalSettings.pureEval;
      nix::evalSettings.pureEval = false;
      this->lockedFlake = std::make_shared<nix::flake::LockedFlake>(
        nix::flake::lockFlake( * this->_state
                             , this->_flakeRef
                             , floxFlakeLockFlags
                             )
      );
      nix::evalSettings.pureEval = oldPurity;
  }
  return this->lockedFlake;
}


/* -------------------------------------------------------------------------- */

  std::list<std::list<std::string>>
FloxFlake::getFlakeAttrPathPrefixes() const
{
  std::list<std::list<std::string>> rsl;
  for ( auto & prefix : this->_prefsPrefixes )
    {
      for ( auto & system : this->_systems )
        {
          if ( prefix == "catalog" )
            {
              for ( auto & stability : this->_prefsStabilities )
                {
                  rsl.push_back( { "catalog", system, stability } );
                }
            }
          else
            {
              rsl.push_back( { prefix, system } );
            }
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

   nix::ref<nix::eval_cache::EvalCache>
FloxFlake::openEvalCache()
{
  nix::flake::Fingerprint fingerprint =
    this->getLockedFlake()->getFingerprint();
  return nix::make_ref<nix::eval_cache::EvalCache>(
    ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
    ? std::optional { std::cref( fingerprint ) }
    : std::nullopt
  , * this->_state
  , [&]()
    {
      nix::Value * vFlake = this->_state->allocValue();
      nix::flake::callFlake(
        * this->_state
      , * this->getLockedFlake()
      , * vFlake
      );
      this->_state->forceAttrs(
        * vFlake, nix::noPos, "while parsing cached flake data"
      );
      nix::Attr * aOutputs = vFlake->attrs->get(
        this->_state->symbols.create( "outputs" )
      );
      assert( aOutputs != nullptr );
      return aOutputs->value;
    }
  );
}


/* -------------------------------------------------------------------------- */

  Cursor
FloxFlake::openCursor( const std::vector<nix::Symbol> & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path ) { cur = cur->getAttr( p ); }
  return cur;
}


/* -------------------------------------------------------------------------- */

  MaybeCursor
FloxFlake::maybeOpenCursor( const std::vector<nix::Symbol> & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path )
    {
      cur = cur->maybeGetAttr( p );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}


/* -------------------------------------------------------------------------- */

  std::list<Cursor>
FloxFlake::getFlakePrefixCursors()
{
  std::list<Cursor>                    rsl;
  nix::ref<nix::eval_cache::EvalCache> cache = this->openEvalCache();
  for ( std::list<std::string> & prefix : this->getFlakeAttrPathPrefixes() )
    {
      MaybeCursor cur = cache->getRoot();
      for ( std::string & p : prefix )
        {
          cur = cur->maybeGetAttr( p );
          if ( cur == nullptr ) { break; }
        }
      if ( cur != nullptr ) { rsl.push_back( Cursor( cur ) ); }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

