/* ========================================================================== *
 *
 * @file flox/search/preferences.cc
 *
 * @brief A set of user inputs used to set input preferences during search.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <vector>

#include <nlohmann/json.hpp>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

/** Preferences used to search for packages in a collection of inputs. */
struct Preferences {

  /** Preferences associated with a named registry input. */
  struct InputPreferences {

    /**
     * Ordered list of subtrees to be searched.
     * Results will be grouped by subtree in the order they appear here.
     */
    std::optional<std::vector<subtree_type>> subtrees;

    /**
     * Ordered list of stabilities to be searched.
     * Catalog results will be grouped by stability in the order they
     * appear here.
     */
    std::optional<std::vector<std::string>> stabilities;

  };  /* End struct `InputPreferences' */


  /**
   * Ordered list of settings associated with specific inputs.
   * Results will be grouped by input in the order they appear here.
   *
   * The identifier `*` is reserved to represent settings which should be
   * used as defaults/fallbacks for any input that doesn't explicitly
   * set a preference.
   */
  std::vector<std::pair<std::string, InputPreferences>> inputs = {};


  /**
   * Ordered list of systems to be searched.
   * Results will be grouped by system in the order they appear here.
   */
  std::vector<std::string> systems;


  /** Allow/disallow packages with certain metadata. */
  struct Allows {

    /** Whether to include packages which are explicitly marked `unfree`. */
    bool unfree = true;

    /** Whether to include packages which are explicitly marked `broken`. */
    bool broken = false;

    /** Filter results to those explicitly marked with the given licenses. */
    std::optional<std::vector<std::string>> licenses;

  } allow;


  /** Settings associated with semantic version processing. */
  struct Semver {

    /** Whether pre-release versions should be ordered before releases. */
    bool preferPreReleases = false;

  } semver;


/* -------------------------------------------------------------------------- */

  /** Reset preferences to default/empty state. */
    void
  clear()
  {
    this->inputs                   = {};
    this->systems                  = {};
    this->allow.unfree             = true;
    this->allow.broken             = false;
    this->allow.licenses           = std::nullopt;
    this->semver.preferPreReleases = false;
  }


  /**
   * Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to lookup
   * packages in a particular input.
   * @param input The input name to be searched.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs & fillQueryArgs( std::string_view      input
                                     , pkgdb::PkgQueryArgs & pqa
                                     ) const;


};  /* End struct `Preferences' */


/* -------------------------------------------------------------------------- */

  static void
from_json( const nlohmann::json & jfrom, Preferences::InputPreferences & prefs )
{
  prefs.subtrees    = std::nullopt;
  prefs.stabilities = std::nullopt;
  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "subtrees" )
        {
          for ( const auto & subtree : value )
            {
              prefs.subtrees = std::vector<subtree_type>();
              prefs.subtrees->emplace_back( subtree.get<subtree_type>() );
            }
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


  static void
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
