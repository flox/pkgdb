/* ========================================================================== *
 *
 * @file search/preferences.cc
 *
 * @brief A set of user inputs used to set input preferences during search.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/search/preferences.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

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
      const SearchParams::RegistryInput & minput =
        this->registry.inputs.at( input );
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
from_json( const nlohmann::json                 & jfrom
         ,       SearchParams::InputPreferences & prefs
         )
{
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( ( key == "subtrees" ) && ( ! value.is_null() ) )
        {
          prefs.subtrees = (std::vector<subtree_type>) value;
        }
      else if ( key == "stabilities" )
        {
          prefs.stabilities = value;
        }
    }
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, SearchParams::RegistryInput & rip )
{
  from_json( jfrom, (SearchParams::InputPreferences &) rip );
  rip.from = std::make_shared<nix::FlakeRef>(
    jfrom.at( "from" ).get<nix::FlakeRef>()
  );
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
          for ( const auto & [rkey, rvalue] : value.items() )
            {
              if ( rkey == "inputs" )
                {
                  params.registry.inputs = rvalue;
                }
              else if ( rkey == "defaults" )
                {
                  params.registry.defaults = rvalue;
                }
              else if ( rkey == "priority" )
                {
                  params.registry.priority = rvalue;
                }
            }
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
