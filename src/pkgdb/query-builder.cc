/* ========================================================================== *
 *
 * @file pkgdb/query-builder.cc
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#include <sql-builder/sql.hh>

#include "flox/pkgdb/query-builder.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb {

/* -------------------------------------------------------------------------- */

  std::string
buildPkgQuery( PkgQueryArgs && params )
{
  using namespace sql;
  // TODO: validate params
  SelectModel q;
  q.select( "id" ).from( "Packages" );

  if ( params.name.has_value() )
    {
      q.where( column( "name" ) == ":name" );
    }

  if ( params.pname.has_value() )
    {
      q.where( column( "pname" ) == ":pname" );
    }
  
  if ( params.match.has_value() && !params.match->empty() )
    {
      q.where( "( name LIKE '%:match%' ) OR ( description LIKE '%:match%' )" );
    }

  if ( params.version.has_value() )
    {
      q.where( column( "version" ) == ":version" );
    }

  // TODO: validate license names, this is a PITA to handle with bind.
  if ( params.licenses.has_value() )
    {
      q.where( column( "license" ).in( params.licenses.value() ) );
    }

  if ( ! params.allowBroken )
    {
      q.where( column( "broken" ).is_null() or ( column( "broken" ) == 0 ) );
    }

  if ( ! params.allowUnfree )
    {
      q.where( column( "unfree" ).is_null() or ( column( "unfree" ) == 0 ) );
    }

  // TODO: stabilities
  // TODO: semver and pre-releases
  // TODO: bind

  return q.str();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
