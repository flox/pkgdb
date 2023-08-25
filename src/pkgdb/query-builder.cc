/* ========================================================================== *
 *
 * @file pkgdb/query-builder.cc
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#include <sstream>

#include <sql-builder/sql.hh>

#include "flox/pkgdb.hh"
#include "flox/pkgdb/query-builder.hh"


/* -------------------------------------------------------------------------- */

namespace flox::pkgdb {

/* -------------------------------------------------------------------------- */

  std::string
PkgQueryArgs::PkgQueryInvalidArgException::errorMessage(
  const PkgQueryArgs::PkgQueryInvalidArgException::error_code & ec
)
{
  switch ( ec )
    {
      case PQEC_MIX_NAME:
        return "Queries may not mix `name' parameter with any of `pname', "
               "`version', or `semver' parameters";
        break;
      case PQEC_MIX_VERSION_SEMVER:
        return "Queries may not mix `version' and `semver' parameters";
        break;
      case PQEC_INVALID_SEMVER:
        return "Semver Parse Error";
        break;
      case PQEC_INVALID_LICENSE:
        return "Query `license' parameter contains invalid character \"'\"";
        break;
      case PQEC_INVALID_SUBTREE:
        return "Unrecognized subtree in query arguments";
        break;
      case PQEC_CONFLICTING_SUBTREE:
        return "Query `stability' parameter may only be used in "
               "\"catalog\" `subtree'";
        break;
      case PQEC_INVALID_SYSTEM:
        return "Unrecognized or unsupported `system' in query arguments";
        break;
      case PQEC_INVALID_STABILITY:
        return "Unrecognized `stability' in query arguments";
        break;
      default:
      case PQEC_ERROR:
        return "Encountered and error processing query arguments";
        break;
    }
}


/* -------------------------------------------------------------------------- */

  std::optional<PkgQueryArgs::PkgQueryInvalidArgException::error_code>
PkgQueryArgs::validate() const
{
  using error_code = PkgQueryArgs::PkgQueryInvalidArgException::error_code;

  if ( this->name.has_value() &&
       ( this->pname.has_value()   ||
         this->version.has_value() ||
         this->semver.has_value()
       )
     )
    {
      return error_code::PQEC_MIX_NAME;
    }

  if ( this->version.has_value() && this->semver.has_value() )
    {
      return error_code::PQEC_MIX_VERSION_SEMVER;
    }

  /* Check licenses don't contain the ' character */
  if ( this->licenses.has_value() )
    {
      for ( const auto & l : * this->licenses )
        {
          if ( l.find( '\'' ) != l.npos )
            {
              return error_code::PQEC_INVALID_LICENSE;
            }
        }
    }

  /* Systems */
  for ( const auto & s : this->systems )
    {
      if ( std::find( flox::defaultSystems.begin()
                    , flox::defaultSystems.end()
                    , s
                    )
           == flox::defaultSystems.end()
         )
        {
          return error_code::PQEC_INVALID_SYSTEM;
        }
    }

  /* Stabilities */
  if ( this->stabilities.has_value() )
    {
      for ( const auto & s : * this->stabilities )
        {
          if ( std::find( flox::defaultCatalogStabilities.begin()
                        , flox::defaultCatalogStabilities.end()
                        , s
                        )
               == flox::defaultCatalogStabilities.end()
             )
            {
              return error_code::PQEC_INVALID_STABILITY;
            }
        }
      if ( ( this->subtrees.has_value() ) &&
           ( ( this->subtrees->size() != 1 ) ||
             ( this->subtrees->front() != ST_CATALOG )
           )
         )
        {
          return error_code::PQEC_CONFLICTING_SUBTREE;
        }
    }

  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

  std::pair<std::string, SQLBinds>
buildPkgQuery( const PkgQueryArgs & params, bool allFields )
{
  /* It's a pain to use `bind' with `in` lists because each element would need
   * its own variable, so instead we scan for unsafe characters and quote
   * /the good ol' fashioned way/ in those cases.
   * Other than that, we use `bind`. */

  using namespace sql;

  /* Validate parameters */
  if ( auto maybe_ec = params.validate(); maybe_ec != std::nullopt )
    {
      throw PkgQueryArgs::PkgQueryInvalidArgException( * maybe_ec );
    }

  SelectModel q;
  q.select( "id" ).select( "semver" ).from( "v_PackagesSearch" );
  std::unordered_map<std::string, std::string> binds;

  /* `sql-builder' only allows `order_by' to be called once, so we have to
   * build the complete `ORDER BY' statement ourselves. */
  bool              firstOrderBy = true;
  std::stringstream orderBy;
  auto addOrderBy =
    [& firstOrderBy, & orderBy]( std::string_view order ) -> void
    {
      if ( firstOrderBy )
        {
          firstOrderBy = false;
          orderBy << order;
        }
      else
        {
          orderBy << ", " << order;
        }
    };

  if ( params.name.has_value() )
    {
      q.where( column( "name" ) == Param( ":name" ) );
      binds.emplace( ":name", * params.name );
    }

  if ( params.pname.has_value() )
    {
      q.where( column( "pname" ) == Param( ":pname" ) );
      binds.emplace( ":pname", * params.pname );
    }
  
  if ( params.match.has_value() && ( ! params.match->empty() ) )
    {
      q.where(
        "( ( pname LIKE :match ) OR ( description LIKE :match ) )"
      );
      /* XXX: These values must align with `match_strength'.
       * While we could use `bind' or `fmt' here, hard-coding them is fine -
       * these are explicitly audited by the test suite.
       * 0 = case-insensitive exact :match with pname
       * 1 = case-insensitive substring :match with pname and description.
       * 2 = case-insensitive substring :match with pname.
       * 3 = case insensitive substring :match with description.
       */
      q.select( R"SQL(
        iif( ( ( '%' || LOWER( pname ) || '%' ) = LOWER( :match ) )
           , 0
           , iif( ( pname LIKE :match )
                , iif( ( description LIKE :match  ), 1, 2 )
                , 3
                )
           ) AS matchStrength
      )SQL" );
      addOrderBy( "matchStrength ASC" );
      binds.emplace( ":match", "%" +  ( * params.match ) + "%" );
    }
  else
    {
      q.select( "NULL AS matchStrength" );
    }

  /* Handle `versionDate' sorting */
  q.select( R"SQL(
    iif( ( versionType != 2 ), NULL
       , date( v_PackagesSearch.version )
       ) AS versionDate
  )SQL" );

  if ( params.version.has_value() )
    {
      q.where( column( "version" ) == Param( ":version" ) );
      binds.emplace( ":version", * params.version );
    }
  else if ( params.semver.has_value() )
    {
      q.where( column( "semver" ).is_not_null() );
    }

  if ( params.licenses.has_value() && ( ! params.licenses->empty() ) )
    {
      q.where( "(" + (
        column( "license" ).is_not_null() and
        column( "license" ).in( * params.licenses )
      ).str() + ")" );
    }

  if ( ! params.allowBroken )
    {
      q.where( "(" +
        ( column( "broken" ).is_null() or ( column( "broken" ) == 0 ) ).str() +
      ")" );
    }

  if ( ! params.allowUnfree )
    {
      q.where( "(" +
        ( column( "unfree" ).is_null() or ( column( "unfree" ) == 0 ) ).str() +
      ")" );
    }

  /* Subtrees */
  if ( params.stabilities.has_value() )
    {
      q.where( column( "subtree" ) == "catalog" );
    }
  else if ( params.subtrees.has_value() )
    {
      size_t                   idx  = 0;
      std::vector<std::string> lst;
      std::stringstream        rank;
      for ( const auto s : * params.subtrees )
        {
          switch ( s )
            {
              case ST_LEGACY:   lst.emplace_back( "legacyPackages" ); break;
              case ST_PACKAGES: lst.emplace_back( "packages" );       break;
              case ST_CATALOG:  lst.emplace_back( "catalog" );        break;
              default:
                throw PkgQueryArgs::PkgQueryInvalidArgException();
                break;
            }
          rank << "iif( ( subtree = '" << lst.back() << "' ), " << idx << ", ";
          ++idx;
        }
      q.where( column( "subtree" ).in( lst ) );
      if ( 1 < idx )
        {
          rank << idx;
          for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
          rank << " AS subtreeRank";
          q.select( rank.str() );
          addOrderBy( "subtreeRank ASC" );
        }
    }

  /* Establish common ordered fields before adding additional optional
   * ranks like `system', and `stability` */
  addOrderBy( "pname ASC" );
  addOrderBy( "versionType ASC" );
  addOrderBy( "major DESC NULLS LAST" );
  addOrderBy( "minor DESC NULLS LAST" );
  addOrderBy( "patch DESC NULLS LAST" );
  addOrderBy( "preTag DESC NULLS LAST" );
  addOrderBy( "versionDate DESC NULLS FIRST" );
  /* Lexicographic as fallback for misc. versions */
  addOrderBy( "v_PackagesSearch.version ASC NULLS LAST" );
  addOrderBy( "brokenRank ASC" );
  addOrderBy( "unfreeRank ASC" );

  /* Stabilities */
  if ( params.stabilities.has_value() )
    {
      q.where( column( "stability" ).in( * params.stabilities ) );
      if ( 1 < params.stabilities->size() )
        {
          size_t            idx  = 0;
          std::stringstream rank;
          for ( const auto & stability : * params.stabilities )
            {
              rank << "iif( ( stability = '" << stability << "' ), " << idx;
              rank << ", ";
              ++idx;
            }
          rank << idx;
          for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
          rank << " AS stabilitiesRank";
          q.select( rank.str() );
          addOrderBy( "stabilitiesRank ASC" );
        }
    }

  /* Systems */
  q.where( column( "system" ).in( params.systems ) );
  if ( 1 < params.systems.size() )
    {
      size_t            idx  = 0;
      std::stringstream rank;
      for ( const auto & system : params.systems )
        {
          rank << "iif( ( system = '" << system << "' ), " << idx << ", ";
          ++idx;
        }
      rank << idx;
      for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
      rank << " AS systemsRank";
      q.select( rank.str() );
      addOrderBy( "systemsRank ASC" );
    }

  /* Finalize `ORDER BY' part */
  q.order_by( orderBy.str() );

  if ( allFields )
    {
      q.select( "subtree" )
       .select( "system" ).select( "stability" ).select( "path" )
       .select( "name" ).select( "version" ).select( "versionType" )
       .select( "major" ).select( "minor" ).select( "patch" ).select( "preTag" )
       .select( "broken" ).select( "brokenRank" ).select( "unfree" )
       .select( "unfreeRank" ).select( "description" );
      return std::make_pair( q.str(), binds );
    }

  /* Removes extra columns used for ordering */
  std::string rsl = "SELECT id, semver FROM ( " + q.str() + " )";
  return std::make_pair( std::move( rsl ), binds );
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
