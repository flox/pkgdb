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
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( qry ) );
  try { jfrom.at( "match" ).get_to( qry.match ); }
  catch( const nlohmann::json::out_of_range & ) {}
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
  this->pkgdb::QueryPreferences::clear();
  this->registry.clear();
  this->query.clear();
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
SearchParams::fillPkgQueryArgs( const std::string         & input
                              ,       pkgdb::PkgQueryArgs & pqa
                              ) const
{
  /* Fill from global preferences */
  this->pkgdb::QueryPreferences::fillPkgQueryArgs( pqa );

  /* Fill from query */
  this->query.fillPkgQueryArgs( pqa );

  /* Fill from input */

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

  return pqa;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & jfrom, SearchParams & params )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::QueryPreferences &>( params ) );
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
      else if ( ( key == "systems" ) ||
                ( key == "allow" )   ||
                ( key == "semver" )
              )
        {
          /* Handled by `QueryPreferences::from_json' */
          continue;
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
  pkgdb::to_json( jto
                , dynamic_cast<const pkgdb::QueryPreferences &>( params )
                );
  jto["registry"] = params.registry;
  jto["query"]    = params.query;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::search' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
