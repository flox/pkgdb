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
  pkgdb::from_json( jfrom, (pkgdb::PkgDescriptorBase &) qry );
  try { jfrom.at( "match" ).get_to( qry.match ); }
  catch( const std::out_of_range & ) {}
}

  void
to_json( nlohmann::json & jfrom, const SearchQuery & qry )
{
  pkgdb::to_json( jfrom, (pkgdb::PkgDescriptorBase &) qry );
  jfrom["match"] = qry.match;
}


/* -------------------------------------------------------------------------- */

  void
SearchParams::clear()
{
  this->registry.clear();
  this->systems                  = {};
  this->allow.unfree             = true;
  this->allow.broken             = false;
  this->allow.licenses           = std::nullopt;
  this->semver.preferPreReleases = false;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
SearchParams::fillQueryArgs( const std::string         & input
                           ,       pkgdb::PkgQueryArgs & pqa
                           ) const
{
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
          params.registry = value;
        }
      else if ( key == "systems" )
        {
          params.systems = value;
        }
      else if ( key == "allow" )
        {
          for ( const auto & [akey, avalue] : value.items() )
            {
              if ( akey == "unfree" )
                {
                  params.allow.unfree   = avalue;
                }
              else if ( akey == "broken" )
                {
                  params.allow.broken   = avalue;
                }
              else if ( akey == "licenses" )
                {
                  params.allow.licenses = avalue;
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
                  params.semver.preferPreReleases = svalue;
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


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
