/* ========================================================================== *
 *
 * @file pkgdb/query-builder.cc
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#include <sql-builder/sql.hh>

#include "flox/pkgdb.hh"
#include "flox/pkgdb/query-builder.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

  namespace pkgdb {

/* -------------------------------------------------------------------------- */

  std::string
PkgQueryArgs::PkgQueryInvalidArgException::errorMessage(
  const PkgQueryArgs::PkgQueryInvalidArgException::error_code & ec
)
{
  switch ( ec )
    {
      case PDQEC_MIX_NAME:
        return "Queries may not mix `name' parameter with any of `pname', "
               "`version', or `semver' parameters";
        break;
      case PDQEC_MIX_VERSION_SEMVER:
        return "Queries may not mix `version' and `semver' parameters";
        break;
      case PDQEC_INVALID_SEMVER:
        return "Semver Parse Error";
        break;
      case PDQEC_INVALID_LICENSE:
        return "Query `license' parameter contains invalid character \"'\"";
        break;
      case PDQEC_INVALID_SUBTREE:
        return "Unrecognized subtree in query arguments";
        break;
      case PDQEC_CONFLICTING_SUBTREE:
        return "Query `stability' parameter may only be used in "
               "\"catalog\" `subtree'";
        break;
      case PDQEC_INVALID_SYSTEM:
        return "Unrecognized or unsupported `system' in query arguments";
        break;
      case PDQEC_INVALID_STABILITY:
        return "Unrecognized `stability' in query arguments";
        break;
      default:
      case PDQEC_ERROR:
        return "Encountered and error processing query arguments";
        break;
    }
}


/* -------------------------------------------------------------------------- */

  std::optional<PkgQueryArgs::PkgQueryInvalidArgException::error_code>
PkgQueryArgs::validate() const
{
  using error_code = PkgQueryArgs::PkgQueryInvalidArgException::error_code;

  // TODO: semver

  if ( this->name.has_value() &&
       ( this->pname.has_value()   ||
         this->version.has_value() ||
         this->semver.has_value()
       )
     )
    {
      return error_code::PDQEC_MIX_NAME;
    }

  if ( this->version.has_value() && this->semver.has_value() )
    {
      return error_code::PDQEC_MIX_VERSION_SEMVER;
    }

  /* Check licenses don't contain the ' character */
  if ( this->licenses.has_value() )
    {
      for ( const auto & l : * this->licenses )
        {
          if ( l.find( '\'' ) != l.npos )
            {
              return error_code::PDQEC_INVALID_LICENSE;
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
          return error_code::PDQEC_INVALID_SYSTEM;
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
              return error_code::PDQEC_INVALID_STABILITY;
            }
        }
      if ( ( this->subtrees.has_value() ) &&
           ( ( this->subtrees->size() != 1 ) ||
             ( this->subtrees->front() != ST_CATALOG )
           )
         )
        {
          return error_code::PDQEC_CONFLICTING_SUBTREE;
        }
    }

  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

  std::pair<std::string, SQLBinds>
buildPkgQuery( const PkgQueryArgs & params )
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
  q.select( "id" ).from( "v_PackagesSearch" );
  std::unordered_map<std::string, std::string> binds;

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
  
  if ( params.match.has_value() && !params.match->empty() )
    {
      q.where( "( name LIKE '%:match%' ) OR ( description LIKE '%:match%' )" );
    }

  if ( params.version.has_value() )
    {
      q.where( column( "version" ) == Param( ":version" ) );
      binds.emplace( ":version", * params.version );
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
      std::vector<std::string> lst;
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
        }
      q.where( column( "subtree" ).in( lst ) );
    }

  /* Systems */
  q.where( column( "system" ).in( params.systems ) );

  /* Stabilities */
  if ( params.stabilities.has_value() )
    {
      if ( params.stabilities->size() == 1 )
        {
          q.where(
            column( "stability" ) == params.stabilities->front()
          );
        }
      else
        {
          q.where( column( "stability" ).in( * params.stabilities ) );
        }
    }

  // TODO: Match
  // TODO: Semver and pre-releases
  // TODO: Sort/"order by" results

  return std::make_pair( q.str(), binds );
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
