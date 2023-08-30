/* ========================================================================== *
 *
 * @file flox/search/preferences.hh
 *
 * @brief A set of user inputs used to set input preferences during search.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <functional>
#include <vector>
#include <ranges>
#include <map>

#include <nlohmann/json.hpp>
#include <nix/flake/flakeref.hh>
#include <nix/fetchers.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

/**
 * @brief SearchParams used to search for packages in a collection of inputs.
 *
 * @example
 * ```
 * {
 *   "registry": {
 *     "inputs": {
 *       "nixpkgs": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "NixOS"
 *         , "repo": "nixpkgs"
 *         }
 *       , "subtrees": ["legacyPackages"]
 *       }
 *     , "floco": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "aakropotkin"
 *         , "repo": "floco"
 *         }
 *       , "subtrees": ["packages"]
 *       }
 *     , "floxpkgs": {
 *         "from": {
 *           "type": "github"
 *         , "owner": "flox"
 *         , "repo": "floxpkgs"
 *         }
 *       , "subtrees": ["catalog"]
 *       , "stabilities": ["stable"]
 *       }
 *     }
 *   , "defaults": {
 *       "subtrees": null
 *     , "stabilities": ["stable"]
 *     }
 *   , "priority": ["nixpkgs", "floco", "floxpkgs"]
 *   }
 * , "systems": ["x86_64-linux"]
 * , "allow":   { "unfree": true, "broken": false, "licenses": ["MIT"] }
 * , "semver":  { "preferPreReleases": false }
 * }
 * ```
 */
struct SearchParams {

/* -------------------------------------------------------------------------- */

  /** SearchParams associated with a registry input. */
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


/* -------------------------------------------------------------------------- */

  /** Preferences associated with a named registry input. */
  struct RegistryInput : public InputPreferences {
    std::shared_ptr<nix::FlakeRef> from;
  };  /* End struct `RegistryInput' */


/* -------------------------------------------------------------------------- */

  /** Settings and fetcher information associated with inputs. */
  struct Registry {

    /** Settings and fetcher information associated with named inputs. */
    std::map<std::string, RegistryInput> inputs;

    /** Default/fallback settings for inputs. */
    InputPreferences defaults;

    /**
     * Priority order used to process inputs.
     * Inputs which do not appear in this list are handled in lexicographical
     * order after any explicitly named inputs.
     */
    std::vector<std::string> priority;

      auto
    getOrder() const
    {
      std::vector<std::reference_wrapper<const std::string>> order(
        this->priority.cbegin()
      , this->priority.cend()
      );
      for ( const auto & [key, _] : this->inputs )
        {
          if ( std::find( this->priority.begin(), this->priority.end(), key )
               == this->priority.end()
             )
            {
              order.emplace_back( key );
            }
        }
      return order;
    }

    /** Reset to default state. */
      inline void
    clear()
    {
      this->inputs.clear();
      this->priority.clear();
    }

  } registry;


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
  void clear();

  /**
   * Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to lookup
   * packages in a particular input.
   * @param input The input name to be searched.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return A reference to the modified query args.
   */
  pkgdb::PkgQueryArgs & fillQueryArgs( const std::string         & input
                                     ,       pkgdb::PkgQueryArgs & pqa
                                     ) const;


};  /* End struct `SearchParams' */


/* -------------------------------------------------------------------------- */

void from_json( const nlohmann::json & jfrom, SearchParams & prefs );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
