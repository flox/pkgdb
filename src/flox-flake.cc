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

#include "flox/flox-flake.hh"
#include "flox/core/util.hh"
#include "flox/core/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

FloxFlake::FloxFlake( const nix::ref<nix::EvalState> & state
                    , const nix::FlakeRef            & ref
                    )
  : state( state )
  , lockedFlake( nix::flake::lockFlake(
      * this->state
    , ref
    , defaultLockFlags
    ) )
{}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::eval_cache::EvalCache>
FloxFlake::openEvalCache()
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


/* -------------------------------------------------------------------------- */

  MaybeCursor
FloxFlake::maybeOpenCursor( const AttrPath & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const auto & part : path )
    {
      cur = cur->maybeGetAttr( part );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}


/* -------------------------------------------------------------------------- */

  Cursor
FloxFlake::openCursor( const AttrPath & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const auto & part : path ) { cur = cur->getAttr( part ); }
  return cur;
}


/* ========================================================================== */

  std::shared_ptr<FloxFlake>
FloxFlakeParserMixin::parseFloxFlake( const std::string & flakeRef )
{
  return std::make_shared<FloxFlake>(
    this->getState()
  , flox::parseFlakeRef( flakeRef )
  );
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<FloxFlake>
FloxFlakeParserMixin::parseFloxFlakeJSON( const nlohmann::json & flakeRef )
{
  auto report = []( std::string_view type ) -> void
  {
    throw FloxException( nix::fmt(
      "Flake references may only be parsed from JSON objects or strings, "
      "but got JSON type '%s'."
    , type
    ) );
  };

  switch ( flakeRef.type() )
    {
      case nlohmann::json::value_t::object:
        return std::make_shared<FloxFlake>(
          this->getState()
        , nix::FlakeRef::fromAttrs( nix::fetchers::jsonToAttrs( flakeRef ) )
        );
        break;
      case nlohmann::json::value_t::string:
        return std::make_shared<FloxFlake>(
          this->getState()
        , nix::parseFlakeRef( flakeRef.get<std::string>() )
        );
        break;
      case nlohmann::json::value_t::null:    report( "null" );    break;
      case nlohmann::json::value_t::array:   report( "array" );   break;
      case nlohmann::json::value_t::boolean: report( "boolean" ); break;
      case nlohmann::json::value_t::number_integer:
      case nlohmann::json::value_t::number_unsigned:
      case nlohmann::json::value_t::number_float:
        report( "number" );
        break;
      default: report( "???" ); break;
    }

  return nullptr;  /* Unreachable */
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
