/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
 *
 * -------------------------------------------------------------------------- */

#include <nlohmann/json.hpp>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include "resolve.hh"
#include "flox/util.hh"
#include <optional>
#include <vector>
#include <map>
#include "flox/predicates.hh"
#include <queue>
#include "flox/drv-cache.hh"
#include "flox/flake-package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

ResolverState::ResolverState(
  const Inputs                 & inputs
, const Preferences            & prefs
, const std::list<std::string> & systems
)
: _prefs( prefs )
{
  /* Increase the default stack size. This aligns with `nix' new CLI usage. */
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
  // TODO: make this an option. It risks making cross-system eval impossible.
  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.setDefault( true );

  for ( auto & id : inputs.getInputNames() )
    {
#if HAVE_BOEHMGC
      this->_inputs.emplace( id, std::allocate_shared<FloxFlake>(
        traceable_allocator<FloxFlake>()
      , this->getEvalState()
      , id
      , inputs.get( id )
      , this->_prefs
      , systems
      ) );
#else
      this->_inputs.emplace( id, std::make_shared<FloxFlake>(
        this->getEvalState()
      , id
      , ref
      , this->_prefs
      , systems
      ) );
#endif
    }
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getStore()
{
  if ( this->_store == nullptr ) { this->_store = nix::openStore(); }
  return nix::ref<nix::Store>( this->_store );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getEvalStore()
{
  if ( this->evalStore == nullptr ) { this->evalStore = this->getStore(); }
  return nix::ref<nix::Store>( this->evalStore );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::EvalState>
ResolverState::getEvalState()
{
  if ( this->evalState == nullptr )
    {
#if HAVE_BOEHMGC
      this->evalState = std::allocate_shared<nix::EvalState>(
        traceable_allocator<nix::EvalState>()
      , std::list<std::string> {}
      , this->getEvalStore()
      , this->getStore()
      );
#else
      this->evalState = std::make_shared<nix::EvalState>(
        std::list<std::string> {}
      , this->getEvalStore()
      , this->getStore()
      );
#endif
      this->evalState->repair = nix::NoRepair;
    }
  return nix::ref<nix::EvalState>( this->evalState );
}


/* -------------------------------------------------------------------------- */

  nix::SymbolTable *
ResolverState::getSymbolTable()
{
  if ( this->evalState == nullptr ) { return & this->getEvalState()->symbols; }
  else                              { return & this->evalState->symbols;      }
}


/* -------------------------------------------------------------------------- */

  std::list<std::string_view>
ResolverState::getInputNames() const
{
  std::list<std::string_view> rsl;
  for ( const auto & [id, _] : this->_inputs ) { rsl.push_back( id ); }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::map<std::string, nix::ref<FloxFlake>>
ResolverState::getInputs() const
{
  std::map<std::string, nix::ref<FloxFlake>> rsl;
  for ( auto & [id, flake] : this->_inputs )
    {
      rsl.emplace( id, nix::ref<FloxFlake>( flake ) );
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::optional<nix::ref<FloxFlake>>
ResolverState::getInput( std::string_view id ) const
{
  for ( auto & [_id, flake] : this->_inputs )
    {
      if ( _id == id  ) { return nix::ref<FloxFlake>( flake ); }
    }
  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

  std::list<Resolved>
ResolverState::resolveInInput( std::string_view id, const Descriptor & desc )
{
  std::string                           _id( id );
  std::list<Resolved>                   results;
  std::queue<Cursor, std::list<Cursor>> todos;

  /**
   * 1. Handling of `id' should have already been handled elsewhere.
   * 2. If we have an `absAttrPath' with no glob we can avoid traversal or
   *    iteration over subtrees and systems.
   * 3. If we have a `relAttrPath' we can avoid a full traversal and just
   *    iterate over subtrees and systesm.
   * 4. Otherwise we have to do a full traversal.
   *    The `packages' output can be optimized slightly by avoiding recursive
   *    descent into `recurseForDerivations' attrs.
   */

  /* Bail early if `id' isn't a match.
   * This should have already been handled by the caller but this will clear
   * existing results in case they do call with a bad input. */
  if ( desc.inputId.has_value() && ( id != desc.inputId.value() ) )
    {
      return {};
    }

  std::shared_ptr<FloxFlake> flake = this->_inputs.at( _id );

  /* Handle `absAttrPath' */
  // TODO: use caches here
  if ( desc.absAttrPath.has_value() )
    {
      if ( desc.absAttrPath.value().hasGlob() )
        {
          std::vector<nix::Symbol> subtree = {
            this->getSymbolTable()->create(
              std::get<std::string>( desc.absAttrPath.value().path[0] )
            )
          };
          Cursor c = flake->openCursor( subtree );
          for ( const std::string & system : flake->getSystems() )
            {
              MaybeCursor s = c->maybeGetAttr( system );
              for ( size_t i = 2;
                    i < desc.absAttrPath.value().path.size();
                    ++i
                  )
                {
                  if ( s == nullptr ) { break; }
                  s = s->maybeGetAttr(
                    std::get<std::string>( desc.absAttrPath.value().path[i] )
                  );
                }
              if ( s != nullptr ) { todos.push( (Cursor) s ); }
            }
        }
      else
        {
          std::vector<nix::Symbol> path;
          for ( size_t i = 0; i < desc.absAttrPath.value().path.size(); ++i )
            {
              this->getSymbolTable()->create(
                std::get<std::string>( desc.absAttrPath.value().path[i] )
              );
            }
          MaybeCursor c = flake->maybeOpenCursor( path );
          if ( c != nullptr ) { todos.push( (Cursor) c ); }
        }
    }
  else if ( desc.relAttrPath.has_value() )
    {
      for ( Cursor c : flake->getFlakePrefixCursors() )
        {
          MaybeCursor m = c;
          for ( const std::string & p : desc.relAttrPath.value() )
            {
              if ( m == nullptr ) { break; }
              m = m->getAttr( p );
            }
          if ( m != nullptr ) { todos.push( (Cursor) m ); }
        }
    }

  predicates::PkgPred pred = this->_prefs.pred_V2();
  pred = pred && desc.pred( todos.empty() );

  std::queue<Package *, std::list<Package *>> goods;

  /* Walk the flake's outputs checking each package. */
  if ( todos.empty() )
    {
      DrvDb cache( flake->getLockedFlake()->getFingerprint() );
      std::list<std::vector<std::string>> tops;

      /* Drop any prefixes that are disabled by our descriptor. */
      for ( Cursor prefix : flake->getFlakePrefixCursors() )
        {
          std::vector<nix::SymbolStr> ppath =
            this->getSymbolTable()->resolve( prefix->getAttrPath() );
          if ( ( ! desc.searchCatalogs ) && ( ppath[0] == "catalog" ) )
            {
              continue;
            }
          else if ( desc.catalogStability.has_value() &&
                    ( desc.catalogStability.value() != ppath[2] )
                  )
            {
              continue;
            }
          if ( ( ! desc.searchFlakes ) &&
               ( ( ppath[0] == "legacyPackages" ) ||
                 ( ppath[0] == "packages" )
               )
             )
            {
              continue;
            }
          todos.push( prefix );
          std::vector<std::string> strs;
          for ( auto & sp : ppath ) { strs.push_back( sp ); }
          tops.push_back( std::move( strs ) );
        }

      // TODO: Use `CachedPackageSet'.
      while ( ! todos.empty() )
        {
          std::vector<nix::Symbol> path = todos.front()->getAttrPath();

          std::string subtree  = ( * this->getSymbolTable() )[path[0]];
          std::string system   = ( * this->getSymbolTable() )[path[1]];
          progress_status dbps = cache.getProgress( subtree, system );

          /* If our cached database is incomplete we evaluate. */
          if ( dbps < DBPS_INFO_DONE )
            {
              /* Mark this prefix as being "in progress". */
              cache.startCommit();
              cache.promoteProgress( subtree, system, DBPS_PARTIAL );

              for ( const nix::Symbol s : todos.front()->getAttrs() )
                {
                  FlakePackage * p = nullptr;
                  try
                    {
                      Cursor c = todos.front()->getAttr( s );
                      if ( c->isDerivation() )
                        {
                          p = new FlakePackage(
                            c, this->getSymbolTable(), false
                          );

                          /* Cache the evaluated result. */
                          cache.setDrvInfo( * p );

                          if ( pred( * p ) ) { goods.push( p ); }
                          else               { delete p;        }
                        }
                      else if ( subtree != "packages" )
                        {
                          MaybeCursor m =
                            c->maybeGetAttr( "recurseForDerivations" );
                          if ( ( m != nullptr ) && m->getBool() )
                            {
                              todos.push( (Cursor) c );
                            }
                        }
                    }
                  catch( ... )
                    {
                      if ( p != nullptr ) { delete p; }
                      // TODO: Catch errors in `packages'.
                    }
                }
              cache.endCommit();
            }
          else  /* If progress is past `DBPS_INFO_DONE' use cached info. */
            {
              std::list<nlohmann::json> infos =
                cache.getDrvInfos( subtree, system );
              for ( const nlohmann::json & info : infos )
                {
                  CachedPackage * p = new CachedPackage( info );
                  if ( pred( * p ) ) { goods.push( p ); }
                  else               { delete p;        }
                }
            }
          /* Move on to the next one. */
          todos.pop();
        }
      /* Mark prefixes as complete in our cache. */
      for ( const std::vector<std::string> absPath : tops )
        {
          cache.setProgress( absPath[0], absPath[1], DBPS_INFO_DONE );
        }
    }
  else  /* Handle case where we have relative/absolute path, so no walking. */
    {
      /* Run our predicate filters and collect satisfactory packages. */
      while( ! todos.empty() )
        {
          if ( todos.front()->isDerivation() )
            {
              FlakePackage * p = new FlakePackage( todos.front()
                                               , this->getSymbolTable()
                                               , false
                                               );
              if ( pred( * p ) ) { goods.push( p ); }
              else               { delete p;        }
            }
          todos.pop();
        }
    }

  /* Convert `Package' to `Resolved'. */
  while ( ! goods.empty() )
    {
      results.push_back( Resolved(
        id
      , flake->getLockedFlakeRef()
      , AttrPathGlob::fromStrings( goods.front()->getPathStrs() )
      , goods.front()->getInfo()
      ) );
      delete goods.front();
      goods.pop();
    }

  mergeResolvedByAttrPathGlob( results );

  /* TODO: Sort by version. This is tricky because of systems. */
  auto sortResults = []( const Resolved & a, const Resolved & b )
  {
    if ( a.path.size() != b.path.size() )
      {
        return a.path.size() <= b.path.size();
      }
    /* Break ties lexicographically. */
    for ( size_t i = 0; i < a.path.size(); ++i )
      {
        if ( i == 1 ) { continue; }  /* Skip system. */
        if ( a.path.path[i] != b.path.path[i] )
          {
            return std::get<std::string>( a.path.path[a.path.size() - 1] ) <=
                   std::get<std::string>( b.path.path[b.path.size() - 1] );
          }
      }
    return true;
  };

  results.sort( sortResults );

  return results;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
