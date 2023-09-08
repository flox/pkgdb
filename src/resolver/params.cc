/* ========================================================================== *
 *
 * @file resolve/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 * parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, PkgDescriptorRaw & desc )
{
  desc.clear();
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( desc ) );
  try { jfrom.at( "preferPreReleases" ).get_to( desc.preferPreReleases ); }
  catch( const std::out_of_range & ) {}
}


  void
to_json( nlohmann::json & jto, const PkgDescriptorRaw & desc )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( desc ) );
  jto["preferPreReleases"] = desc.preferPreReleases;
}


/* -------------------------------------------------------------------------- */

  void
ResolveOneParams::clear()
{
  this->registry.clear();
  this->query.clear();
  this->allow.unfree             = true;
  this->allow.broken             = false;
  this->allow.licenses           = std::nullopt;
  this->semver.preferPreReleases = false;
}


/* -------------------------------------------------------------------------- */

  bool
ResolveOneParams::fillPkgQueryArgs( const std::string         & input
                                  ,       pkgdb::PkgQueryArgs & pqa
                                  ) const
{
  pqa.clear();

  if ( this->query.input.has_value() && ( input != ( * this->query.input ) ) )
    {
      return false;
    }

  // TODO:
  //this->query.fillPkgQueryArgs( pqa );

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

  pqa.systems           = this->systems;
  pqa.allowUnfree       = this->allow.unfree;
  pqa.allowBroken       = this->allow.broken;
  pqa.licenses          = this->allow.licenses;
  pqa.preferPreReleases =
    this->query.preferPreReleases.value_or( this->semver.preferPreReleases );

  return true;
}


/* -------------------------------------------------------------------------- */


  void
from_json( const nlohmann::json & jfrom, ResolveOneParams & params )
{
  params.clear();
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
      else if ( key == "systems" )
        {
          value.get_to( params.systems );
        }
      else if ( key == "allow" )
        {
          for ( const auto & [akey, avalue] : value.items() )
            {
              if ( akey == "unfree" )
                {
                  avalue.get_to( params.allow.unfree );
                }
              else if ( akey == "broken" )
                {
                  avalue.get_to( params.allow.broken );
                }
              else if ( akey == "licenses" )
                {
                  avalue.get_to( params.allow.licenses );
                }
              else
                {
                  throw FloxException(
                    "Unexpected preferences field 'allow." + key + '\''
                  );
                }
            }
        }
      else if ( key == "semver" )
        {
          for ( const auto & [skey, svalue] : value.items() )
            {
              if ( skey == "preferPreReleases" )
                {
                  svalue.get_to( params.semver.preferPreReleases );
                }
              else
                {
                  throw FloxException(
                    "Unexpected preferences field 'semver." + key + '\''
                  );
                }
            }
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
  jto = {
    { "registry", params.registry }
  , { "systems",  params.systems  }
  , { "allow", {
        { "unfree",   params.allow.unfree   }
      , { "broken",   params.allow.broken   }
      , { "licenses", params.allow.licenses }
      }
    }
  , { "semver", { { "preferPreReleases", params.semver.preferPreReleases } } }
  , { "query", params.query }
  };
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
