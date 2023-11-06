/* ========================================================================== *
 *
 * @file search/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during search.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/search/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::search {

/* -------------------------------------------------------------------------- */

void
SearchQuery::clear()
{
  this->pkgdb::PkgDescriptorBase::clear();
  this->partialMatch     = std::nullopt;
  this->partialNameMatch = std::nullopt;
}


/* -------------------------------------------------------------------------- */

void
SearchQuery::check() const
{
  /* `partialMatch' and `partialNameMatch' cannot be used together. */
  if ( this->partialMatch.has_value() && this->partialNameMatch.has_value() )
    {
      throw ParseSearchQueryException(
        "`partialmatch' and `partialNameMatch' filters "
        "may not be used together." );
    }
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, SearchQuery &qry )
{
  auto getOrFail
    = [&]( const std::string &key, const nlohmann::json &from, auto &sink )
  {
    if ( from.is_null() ) { return; }
    try
      {
        from.get_to( sink );
      }
    catch ( const nlohmann::json::exception &err )
      {
        throw ParseSearchQueryException( "parsing field: 'query." + key + "'",
                                         err.what() );
      }
    catch ( ... )
      {
        throw ParseSearchQueryException( "parsing field: 'query." + key
                                         + "'." );
      }
  };

  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "name" ) { getOrFail( key, value, qry.name ); }
      else if ( key == "pname" ) { getOrFail( key, value, qry.pname ); }
      else if ( key == "version" ) { getOrFail( key, value, qry.version ); }
      else if ( key == "semver" ) { getOrFail( key, value, qry.semver ); }
      else if ( key == "match" ) { getOrFail( key, value, qry.partialMatch ); }
      else if ( ( key == "match-name" ) || ( key == "matchName" ) )
        {
          getOrFail( key, value, qry.partialNameMatch );
        }
      else
        {
          throw ParseSearchQueryException( "unrecognized key: 'query." + key
                                           + "'." );
        }
    }
}


void
to_json( nlohmann::json &jto, const SearchQuery &qry )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( qry ) );
  jto["match"]      = qry.partialMatch;
  jto["name-match"] = qry.partialNameMatch;
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
SearchQuery::fillPkgQueryArgs( pkgdb::PkgQueryArgs &pqa ) const
{
  /* XXX: DOES NOT CLEAR FIRST! We are called after global preferences. */
  pqa.name             = this->name;
  pqa.pname            = this->pname;
  pqa.version          = this->version;
  pqa.semver           = this->semver;
  pqa.partialMatch     = this->partialMatch;
  pqa.partialNameMatch = this->partialNameMatch;
  return pqa;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::search


/* -------------------------------------------------------------------------- */

/* Instantiate templates. */
namespace flox::pkgdb {

template struct QueryParams<search::SearchQuery>;

template void
from_json( const nlohmann::json &, QueryParams<search::SearchQuery> & );

template void
to_json( nlohmann::json &, const QueryParams<search::SearchQuery> & );

}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
