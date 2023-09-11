/* ========================================================================== *
 *
 * @file resolve/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/globals.hh>

#include "flox/core/util.hh"
#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

  void
PkgDescriptorRaw::clear()
{
  this->pkgdb::PkgDescriptorBase::clear();
  this->input             = std::nullopt;
  this->path              = std::nullopt;
  this->subtree           = std::nullopt;
  this->stability         = std::nullopt;
  this->preferPreReleases = std::nullopt;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, PkgDescriptorRaw & desc )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( desc ) );
  try { jfrom.at( "preferPreReleases" ).get_to( desc.preferPreReleases ); }
  catch( const nlohmann::json::out_of_range & ) {}
  try { jfrom.at( "path" ).get_to( desc.path ); }
  catch( const nlohmann::json::out_of_range & ) {}
}


  void
to_json( nlohmann::json & jto, const PkgDescriptorRaw & desc )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( desc ) );
  jto["preferPreReleases"] = desc.preferPreReleases;
  jto["path"] = desc.path;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
PkgDescriptorRaw::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
{
  /* XXX: DOES NOT CLEAR FIRST! We are called after global preferences. */
  pqa.name    = this->name;
  pqa.pname   = this->pname;
  pqa.version = this->version;
  pqa.semver  = this->semver;

  /* Set pre-release preference. We get priority over _global_ preferences. */
  if ( this->preferPreReleases.has_value() )
    {
      pqa.preferPreReleases = * this->preferPreReleases;
    }

  /* Handle `path' parameter. */
  if ( this->path.has_value() && ( ! this->path->empty() ) )
    {
      flox::AttrPath relPath;
      auto fillRelPath = [&]( int start )
        {
          std::transform( this->path->begin() + start
                        , this->path->end()
                        , std::back_inserter( relPath )
                        , []( const auto & s ) { return * s; }
                        );
        };
      /* If path is absolute set `subtree', `systems', and `stabilities'. */
      if ( std::find( getDefaultSubtrees().begin()
                    , getDefaultSubtrees().end()
                    , this->path->front()
                    ) != getDefaultSubtrees().end()
         )
        {
          if ( this->path->size() < 3 )
            {
              throw FloxException(
                "Absolute attribute paths must have at least three elements"
              );
            }
          subtree_type subtree;
          from_json( * this->path->front(), subtree );
          if ( this->path->at( 1 ).has_value() )
            {
              pqa.systems = std::vector<std::string> { * this->path->at( 1 ) };
            }

          if ( subtree == ST_CATALOG )
            {
              if ( this->path->size() < 4 )
                {
                  throw FloxException(
                    "Absolute attribute paths in catalogs must have at "
                    "least four elements"
                  );
                }
              pqa.stabilities =
                std::vector<std::string> { * this->path->at( 2 ) };
              fillRelPath( 3 );
            }
          else
            {
              fillRelPath( 2 );
            }
          pqa.subtrees = std::vector<subtree_type> { std::move( subtree ) };
        }
      else
        {
          fillRelPath( 0 );
        }
      pqa.relPath = std::move( relPath );
    }

  return pqa;
}


/* -------------------------------------------------------------------------- */

  void
ResolveOneParams::clear()
{
  this->pkgdb::QueryPreferences::clear();
  this->registry.clear();
  this->query.clear();
}


/* -------------------------------------------------------------------------- */

  bool
ResolveOneParams::fillPkgQueryArgs( const std::string         & input
                                  ,       pkgdb::PkgQueryArgs & pqa
                                  ) const
{
  if ( this->query.input.has_value() && ( input != ( * this->query.input ) ) )
    {
      return false;
    }

  /* Fill from globals */
  this->pkgdb::QueryPreferences::fillPkgQueryArgs( pqa );

  /* Fill from query */
  this->query.fillPkgQueryArgs( pqa );

  /* Fill from input */

  /* Look for the named input and our fallbacks/default in the inputs list.
   * then fill input specific settings. */
  try
    {
      const RegistryInput & minput = this->registry.inputs.at( input );
      pqa.subtrees = minput.subtrees.has_value()
                     ? minput.subtrees
                     : this->registry.defaults.subtrees;
      pqa.stabilities = minput.stabilities.has_value()
                        ? minput.stabilities
                        : this->registry.defaults.stabilities;
    }
  catch ( ... )
    {
      pqa.subtrees    = this->registry.defaults.subtrees;
      pqa.stabilities = this->registry.defaults.stabilities;
    }

  /* If there's a stability specified by the query we have extra work to do.  */
  if ( this->query.stability.has_value() )
    {
      /* If the input lacks this stability, skip. */
      if ( ( pqa.stabilities.has_value() ) &&
           ( std::find( pqa.stabilities->begin()
                      , pqa.stabilities->end()
                      , * this->query.stability
                      ) == pqa.stabilities->end()
           )
         )
        {
          pqa.clear();
          return false;
        }
      /* Otherwise, force catalog resolution. */
      pqa.subtrees = std::vector<subtree_type> { ST_CATALOG };
    }

  /* If the input lacks the subtree we need then skip. */
  if ( pqa.subtrees.has_value() && ( this->query.subtree.has_value() ) )
    {
      subtree_type subtree;
      from_json( * this->query.subtree, subtree );
      if ( std::find( pqa.subtrees->begin()
                    , pqa.subtrees->end()
                    , subtree
                    ) == pqa.subtrees->end()
         )
        {
          pqa.clear();
          return false;
        }
    }

  return true;
}


/* -------------------------------------------------------------------------- */


  void
from_json( const nlohmann::json & jfrom, ResolveOneParams & params )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::QueryPreferences &>( params ) );
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "registry" )
        {
          value.get_to( params.registry );
        }
      else if ( key == "query" )
        {
          value.get_to( params.query );
        }
      else if ( ( key == "systems" ) ||
                ( key == "allow" )   ||
                ( key == "semver" )
              )
        {
          /* Handled by `QueryPreferences::from_json' */
          continue;
        }
      else
        {
          throw FloxException( "Unexpected preferences field '" + key + '\'' );
        }
    }
}


  void
to_json( nlohmann::json & jto, const ResolveOneParams & params )
{
  pkgdb::to_json( jto
                , dynamic_cast<const pkgdb::QueryPreferences &>( params )
                );
  jto["registry"] = params.registry;
  jto["query"]    = params.query;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */