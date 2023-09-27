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
  this->partialMatch = std::nullopt;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, SearchQuery & qry )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( qry ) );
  try { jfrom.at( "partialMatch" ).get_to( qry.partialMatch ); }
  catch( const nlohmann::json::out_of_range & ) {}
}

  void
to_json( nlohmann::json & jto, const SearchQuery & qry )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( qry ) );
  jto["partialMatch"] = qry.partialMatch;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
SearchQuery::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
{
  /* XXX: DOES NOT CLEAR FIRST! We are called after global preferences. */
  pqa.name               = this->name;
  pqa.pname              = this->pname;
  pqa.version            = this->version;
  pqa.semver             = this->semver;
  pqa.partialMatch       = this->partialMatch;
  return pqa;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- */

/* Instantiate templates. */
namespace flox::pkgdb {
  template struct QueryParams<search::SearchQuery>;
  template void from_json( const nlohmann::json                   &
                         ,       QueryParams<search::SearchQuery> &
                         );
  template void to_json(       nlohmann::json                   &
                       , const QueryParams<search::SearchQuery> &
                       );
}  /* End namespaces `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
