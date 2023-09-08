/* ========================================================================== *
 *
 * @file search/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 * parameters during search.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, SearchQuery & qry )
{
  qry.clear();
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( qry ) );
  try { jfrom.at( "match" ).get_to( qry.match ); }
  catch( const std::out_of_range & ) {}
}

  void
to_json( nlohmann::json & jto, const SearchQuery & qry )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( qry ) );
  jto["match"] = qry.match;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
SearchQuery::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
{
  pqa.clear();
  pqa.name    = this->name;
  pqa.pname   = this->pname;
  pqa.version = this->version;
  pqa.semver  = this->semver;
  pqa.match   = this->match;
  return pqa;
}


/* -------------------------------------------------------------------------- */

  void
SearchParams::clear()
{
  this->registry.clear();
  this->query.clear();
  this->systems                  = {};
  this->allow.unfree             = true;
  this->allow.broken             = false;
  this->allow.licenses           = std::nullopt;
  this->semver.preferPreReleases = false;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
SearchParams::fillPkgQueryArgs( const std::string         & input
                              ,       pkgdb::PkgQueryArgs & pqa
                              ) const
{
  pqa.clear();
  this->query.fillPkgQueryArgs( pqa );

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

  pqa.systems           = this->systems;
  pqa.allowUnfree       = this->allow.unfree;
  pqa.allowBroken       = this->allow.broken;
  pqa.licenses          = this->allow.licenses;
  pqa.preferPreReleases = this->semver.preferPreReleases;
  return pqa;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, SearchParams & params )
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
to_json( nlohmann::json & jto, const SearchParams & params )
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

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
