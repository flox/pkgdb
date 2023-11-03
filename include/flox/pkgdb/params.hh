/* ========================================================================== *
 *
 * @file flox/pkgdb/params.hh
 *
 * @brief User settings used to query a package database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nlohmann/json.hpp>

#include "flox/pkgdb/pkg-query.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

/**
 * @brief Global preferences used for resolution/search with multiple queries.
 */
struct QueryPreferences
{

  /**
   * Ordered list of systems to be searched.
   * Results will be grouped by system in the order they appear here.
   *
   * Defaults to the current system.
   */
  std::vector<std::string> systems = { nix::settings.thisSystem.get() };


  /** @brief Allow/disallow packages with certain metadata. */
  struct Allows
  {

    /** Whether to include packages which are explicitly marked `unfree`. */
    bool unfree = true;

    /** Whether to include packages which are explicitly marked `broken`. */
    bool broken = false;

    /** Filter results to those explicitly marked with the given licenses. */
    std::optional<std::vector<std::string>> licenses;

  }; /* End struct `QueryPreferences::Allows' */

  Allows allow; /**< Allow/disallow packages with certain metadata. */


  /**
   * @brief Settings associated with semantic version processing.
   *
   * These act as the _global_ default, but may be overridden by
   * individual descriptors.
   */
  struct Semver
  {
    /** Whether pre-release versions should be ordered before releases. */
    bool preferPreReleases = false;
  }; /* End struct `QueryPreferences::Semver' */

  Semver semver; /**< Settings associated with semantic version processing. */


  /** @brief Reset to default/empty state. */
  virtual void
  clear();

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages.
   *
   * NOTE: This DOES clear @a pqa before filling it.
   */
  pkgdb::PkgQueryArgs &
  fillPkgQueryArgs( pkgdb::PkgQueryArgs &pqa ) const;


}; /* End struct `QueryPreferences' */


/* -------------------------------------------------------------------------- */

/**
 * @class flox::pkgdb::ParseQueryPreferencesException
 * @brief An exception thrown when parsing @a flox::pkgdb::QueryPreferences
 *        from JSON.
 *
 * @{
 */
FLOX_DEFINE_EXCEPTION( ParseQueryPreferencesException,
                       EC_PARSE_QUERY_PREFERENCES,
                       "error parsing query preferences" )
/** @} */


/* -------------------------------------------------------------------------- */

/**
 * @brief Convert a JSON object to a @a flox::pkgdb::QueryPreferences.
 *
 * NOTE: This DOES clear @a prefs before filling it.
 * NOTE: Does not `throw` for unknown keys at the top level.
 */
void
from_json( const nlohmann::json &jfrom, QueryPreferences &prefs );

/**
 * @brief Convert a @a flox::pkgdb::QueryPreferences to a JSON object.
 *
 * NOTE: This DOES clear @a jto before filling it.
 */
void
to_json( nlohmann::json &jto, const QueryPreferences &prefs );


/* -------------------------------------------------------------------------- */

/**
 * @brief A set of query parameters for resolving a single descriptor.
 *
 * This is a trivially simple form of resolution which does not consider
 * _groups_ of descriptors or attempt to optimize with additional context.
 *
 * This is essentially a reorganized form of @a flox::pkgdb::PkgQueryArgs
 * that is suited for JSON input.
 *
 * @see flox::resolver::ResolveOneParams
 * @see flox::search::SearchParams
 */
template<pkg_descriptor_typename QueryType>
struct QueryParams : public QueryPreferences
{

  using query_type = QueryType;

  /* From `QueryPreferences':
   *   std::vector<std::string> systems;
   *   struct Allows {
   *     bool unfree = true;
   *     bool broken = false;
   *     std::optional<std::vector<std::string>> licenses;
   *   } allow;
   *   struct Semver {
   *     preferPreReleases = false;
   *   } semver;
   */

  /** Settings and fetcher information associated with inputs. */
  RegistryRaw registry;

  /**
   * @brief A single package descriptor in _raw_ form.
   *
   * This requires additional post-processing, such as "pushing down" global
   * settings, before it can be used to perform resolution.
   */
  QueryType query;

  /** @brief Reset to default/empty state. */
  virtual void
  clear() override
  {
    this->pkgdb::QueryPreferences::clear();
    this->registry.clear();
    this->query.clear();
  }

  /**
   * @brief Fill a @a flox::pkgdb::PkgQueryArgs struct with preferences to
   *        lookup packages in a particular input.
   * @param input The input name to be searched.
   * @param pqa   A set of query args to _fill_ with preferences.
   * @return `true` if @a pqa was modified, indicating that the input should be
   *         searched, `false` otherwise.
   */
  virtual bool
  fillPkgQueryArgs( const std::string &input, pkgdb::PkgQueryArgs &pqa ) const
  {
    /* Fill from globals */
    this->pkgdb::QueryPreferences::fillPkgQueryArgs( pqa );
    /* Fill from input */
    this->registry.fillPkgQueryArgs( input, pqa );
    /* Fill from query */
    this->query.fillPkgQueryArgs( pqa );
    return true;
  }


}; /* End struct `ResolveOneParams' */


/* -------------------------------------------------------------------------- */

/**
 * @class flox::pkgdb::ParseQueryParamsException
 * @brief An exception thrown when parsing @a flox::pkgdb::QueryParams
 *        from JSON.
 *
 * @{
 */
FLOX_DEFINE_EXCEPTION( ParseQueryParamsException,
                       EC_PARSE_QUERY_PARAMS,
                       "error parsing query parameters" )
/** @} */


/* -------------------------------------------------------------------------- */

template<pkg_descriptor_typename QueryType>
void
from_json( const nlohmann::json &jfrom, QueryParams<QueryType> &params )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::QueryPreferences &>( params ) );
  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "registry" )
        {
          if ( value.is_null() ) { continue; }
          value.get_to( params.registry );
        }
      else if ( key == "query" )
        {
          if ( value.is_null() ) { continue; }
          value.get_to( params.query );
        }
      else if ( ( key == "systems" ) || ( key == "allow" )
                || ( key == "semver" ) )
        {
          /* Handled by `QueryPreferences::from_json' */
          continue;
        }
      else
        {
          throw ParseQueryParamsException( "unexpected preferences field '"
                                           + key + '\'' );
        }
    }
}


/* -------------------------------------------------------------------------- */

template<pkg_descriptor_typename QueryType>
void
to_json( nlohmann::json &jto, const QueryParams<QueryType> &params )
{
  pkgdb::to_json( jto,
                  dynamic_cast<const pkgdb::QueryPreferences &>( params ) );
  jto["registry"] = params.registry;
  jto["query"]    = params.query;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
