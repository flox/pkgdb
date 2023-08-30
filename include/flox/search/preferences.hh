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
  void clear();

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

void from_json( const nlohmann::json                & jfrom
              ,       Preferences::InputPreferences & prefs
              );

void from_json( const nlohmann::json & jfrom, Preferences & prefs );


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
