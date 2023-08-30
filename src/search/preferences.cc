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
Preferences::clear()
{
  this->inputs                   = {};
  this->systems                  = {};
  this->allow.unfree             = true;
  this->allow.broken             = false;
  this->allow.licenses           = std::nullopt;
  this->semver.preferPreReleases = false;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
Preferences::fillQueryArgs( std::string_view      input
                          , pkgdb::PkgQueryArgs & pqa
                          ) const
{
  /* Look for the named input and our fallbacks/default in the inputs list. */
  std::optional<Preferences::InputPreferences> dft;
  std::optional<Preferences::InputPreferences> prefs;
  for ( const auto & elem : this->inputs )
    {
      if ( elem.first == input )    { prefs = elem.second; }
      else if ( elem.first == "*" ) { dft   = elem.second; }
    }

  /* Try filling input specific settings. */
  if ( dft.has_value() )
    {
      if ( prefs.has_value() )
        {
          pqa.subtrees = prefs->subtrees.has_value() ? prefs->subtrees
                                                     : dft->subtrees;
          pqa.stabilities = prefs->stabilities.has_value() ? prefs->stabilities
                                                           : dft->stabilities;
        }
      else if ( prefs.has_value() )
        {
          pqa.subtrees    = prefs->subtrees;
          pqa.stabilities = prefs->stabilities;
        }
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
from_json( const nlohmann::json & jfrom, Preferences::InputPreferences & prefs )
{
  prefs.subtrees    = std::nullopt;
  prefs.stabilities = std::nullopt;
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "subtrees" )
        {
          prefs.subtrees = (std::vector<subtree_type>) value;
        }
      else if ( key == "stabilities" )
        {
          prefs.stabilities = value;
        }
      else
        {
          throw FloxException(
            "Unexpected preferences field 'inputs.*." + key + '\''
          );
        }
    }
}


  void
from_json( const nlohmann::json & jfrom, Preferences & prefs )
{
  prefs.clear();
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "inputs" )
        {
          /* A list of `[{ "<NAME>": {..} }, ...]' plists. */
          for ( const auto & input : value )
            {
              std::string name = input.items().begin().key();
              /* Make sure it hasn't been declared yet. */
              for ( const auto & pinput : prefs.inputs )
                {
                  if ( name == pinput.first )
                    {
                      throw FloxException(
                        "Input '" + name + "' declared multiple times"
                      );
                    }
                }
              prefs.inputs.emplace_back( name, input.items().begin().value() );
            }
        }
      else if ( key == "systems" )
        {
          prefs.systems = value;
        }
      else if ( key == "allow" )
        {
          for ( const auto & [akey, avalue] : value.items() )
            {
              if ( akey == "unfree" )
                {
                  prefs.allow.unfree   = avalue;
                }
              else if ( akey == "broken" )
                {
                  prefs.allow.broken   = avalue;
                }
              else if ( akey == "licenses" )
                {
                  prefs.allow.licenses = avalue;
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
                  prefs.semver.preferPreReleases = svalue;
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
