/* ========================================================================== *
 *
 * @file pkgdb/pkg-query.cc
 *
 * @brief Interfaces for constructing complex `Packages' queries.
 *
 *
 * -------------------------------------------------------------------------- */

#include <sstream>
#include <set>
#include <list>

#include <sql-builder/sql.hh>

#include "flox/pkgdb.hh"
#include "flox/pkgdb/pkg-query.hh"


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
    }

  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

  void
PkgQuery::addSelection( std::string_view column )
{
  if ( this->firstSelect ) { this->firstSelect = false; }
  else                     { this->selects << ", ";     }
  this->selects << column;
}

  void
PkgQuery::addOrderBy( std::string_view order )
{
  if ( this->firstOrder ) { this->firstOrder = false; }
  else                    { this->orders << ", ";     }
  this->orders << order;
}

  void
PkgQuery::addWhere( std::string_view cond )
{
  if ( this->firstWhere ) { this->firstWhere = false; }
  else                    { this->wheres << " AND ";     }
  this->wheres << "( " << cond << " )";
}


/* -------------------------------------------------------------------------- */

  void
PkgQuery::clearBuilt()
{
  this->selects.clear();
  this->orders.clear();
  this->wheres.clear();
  this->firstSelect = true;
  this->firstOrder  = true;
  this->firstWhere  = true;
  this->binds       = {};
}


/* -------------------------------------------------------------------------- */

  static void
addIn( std::stringstream & oss, const std::vector<std::string> & elems )
{
  oss << " IN ( ";
  bool first = true;
  for ( const auto & elem : elems )
    {
      if ( first ) { first = false; } else { oss << ", "; }
      oss << '\'' << elem << '\'';
    }
  oss << " )";
}


/* -------------------------------------------------------------------------- */

  void
PkgQuery::init()
{
  this->clearBuilt();

  /* Validate parameters */
  if ( auto maybe_ec = this->validate(); maybe_ec != std::nullopt )
    {
      throw PkgQueryArgs::PkgQueryInvalidArgException( * maybe_ec );
    }

  this->addSelection( "*" );

  /* Handle fuzzy matching filtering. */
  if ( this->match.has_value() && ( ! this->match->empty() ) )
    {
      this->addWhere(
        "( pname LIKE :match ) OR ( description LIKE :match )"
      );

      /* We _rank_ the strength of a match from 0-3 based on exact and partial
       * matches of `pname` and `description` columns.
       * Because we intend to use `bind` to handle quoting the user's match
       * string, we add `%<STRING>%` before binding so that `LIKE` works, and
       * need to manually add those characters below when testing for exact
       * matches of the `pname` column.
       * - 0 : Exact `pname` match.
       * - 1 : Partial matches on both `pname` and `description`.
       * - 2 : Partial match on `pname`.
       * - 3 : Partial match on `description`.
       * Our `WHERE` statement above ensures that at least one of these rankings
       * will be true forall rows.
       */
      std::stringstream matcher;
      matcher << "iif( ( ( '%' || LOWER( pname ) || '%' ) = LOWER( :match ) )"
              << ", " << MS_EXACT_PNAME
              << ", iif( ( pname LIKE :match )"
              << ", iif( ( description LIKE :match  ), "
              << MS_PARTIAL_PNAME_DESC << ", "
              << MS_PARTIAL_PNAME << " ), " << MS_PARTIAL_DESC
              << " ) ) AS matchStrength";
      this->addSelection( matcher.str() );
      /* Add `%` before binding so `LIKE` works. */
      binds.emplace( ":match", "%" +  ( * this->match ) + "%" );
    }
  else
    {
      /* Add a bogus `matchStrength` so that later `ORDER BY` works. */
      std::stringstream matcher;
      matcher << MS_NONE << " AS matchStrength";
      this->addSelection( matcher.str() );
    }

  /* Handle `pname' filtering. */
  if ( this->pname.has_value() )
    {
      this->addWhere( "pname = :pname" );
      this->binds.emplace( ":pname", * this->pname );
    }

  /* Handle `version' and `semver' filtering.  */
  if ( this->version.has_value() )
    {
      this->addWhere( "version = :version" );
      this->binds.emplace( ":version", * this->version );
    }
  else if ( this->semver.has_value() )
    {
      this->addWhere( "semver IS NOT NULL" );
    }

  /* Handle `licenses' filtering. */
  if ( this->licenses.has_value() && ( ! this->licenses->empty() ) )
    {
      this->addWhere( "license IS NOT NULL" );
      /* licenses IN ( ... ) */
      std::stringstream cond;
      cond << "license";
      addIn( cond, * this->licenses );
      this->addWhere( cond.str() );
    }

  /* Handle `broken' filtering. */
  if ( ! this->allowBroken )
    {
      this->addWhere( "( broken IS NULL ) OR ( broken = FALSE )" );
    }

  /* Handle `unfree' filtering. */
  if ( ! this->allowUnfree )
    {
      this->addWhere( "( unfree IS NULL ) OR ( unfree = FALSE )" );
    }

  /* Handle `subtrees' filtering. */
  if ( this->subtrees.has_value() )
    {
      size_t                   idx  = 0;
      std::vector<std::string> lst;
      std::stringstream        rank;
      for ( const auto s : * this->subtrees )
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
      /* subtree IN ( ...  ) */
      std::stringstream cond;
      cond << "subtree";
      addIn( cond, lst );
      this->addWhere( cond.str() );
      /* Wrap up rankings assignment. */
      if ( 1 < idx )
        {
          rank << idx;
          for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
          rank << " AS subtreesRank";
          this->addSelection( rank.str() );
        }
      else
        {
          /* Add bogus rank so `ORDER BY subtreesRank' works. */
          this->addSelection( "0 AS subtreesRank" );
        }
    }
  else
    {
      /* Add bogus rank so `ORDER BY subtreesRank' works. */
      this->addSelection( "0 AS subtreesRank" );
    }

  /* Handle `systems' filtering. */
  {
    std::stringstream cond;
    cond << "system";
    addIn( cond, this->systems );
    this->addWhere( cond.str() );
  }
  if ( 1 < this->systems.size() )
    {
      size_t            idx  = 0;
      std::stringstream rank;
      for ( const auto & system : this->systems )
        {
          rank << "iif( ( system = '" << system << "' ), " << idx << ", ";
          ++idx;
        }
      rank << idx;
      for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
      rank << " AS systemsRank";
      this->addSelection( rank.str() );
    }
  else
    {
      /* Add a bogus rank to `ORDER BY systemsRank' works. */
      this->addSelection( "0 AS systemsRank" );
    }

  /* Handle `stabilities' filtering. */
  if ( this->stabilities.has_value() )
    {
      std::stringstream cond;
      cond << "( stability IS NULL ) OR ( stability ";
      addIn( cond, * this->stabilities );
      cond << " )";
      this->addWhere( cond.str() );
      if ( 1 < this->stabilities->size() )
        {
          size_t            idx  = 0;
          std::stringstream rank;
          rank << "iif( ( stability IS NULL ), NULL, ";
          for ( const auto & stability : * this->stabilities )
            {
              rank << "iif( ( stability = '" << stability << "' ), " << idx;
              rank << ", ";
              ++idx;
            }
          rank << idx;
          for ( size_t i = 0; i < idx; ++i ) { rank << " )"; }
          rank << " ) AS stabilitiesRank";
          this->addSelection( rank.str() );
        }
      else
        {
          /* Add a bogus rank so `ORDER BY stabilitiesRank' works. */
          this->addSelection( "0 AS stabilitiesRank" );
        }
    }
  else
    {
      /* Add a bogus rank so `ORDER BY stabilitiesRank' works. */
      this->addSelection( "0 AS stabilitiesRank" );
    }

  /* Establish ordering. */
  this->addOrderBy( R"SQL(
    matchStrength ASC
  , subtreesRank ASC
  , systemsRank ASC
  , stabilitiesRank ASC NULLS LAST
  , pname ASC
  , major DESC NULLS LAST
  , minor DESC NULLS LAST
  , patch DESC NULLS LAST
  )SQL" );
  /* Handle `preferPreReleases' */
  if ( this->preferPreReleases )
    {
      this->addOrderBy( "preTag DESC NULLS LAST" );
    }
  else
    {
      this->addOrderBy( "preTag DESC NULLS FIRST" );
    }
  this->addOrderBy( R"SQL(
    versionDate DESC NULLS LAST
  -- Lexicographic as fallback for misc. versions
  , v_PackagesSearch.version ASC NULLS LAST
  , brokenRank ASC
  , unfreeRank ASC
  )SQL" );

}


/* -------------------------------------------------------------------------- */

  std::string
PkgQuery::str() const
{
  std::stringstream qry;
  qry << "SELECT ";
  bool firstExport = true;
  for ( const auto & column : this->exportedColumns )
    {
      if ( firstExport ) { firstExport = false; } else { qry << ", "; }
      qry << column;
    }
  qry << " FROM ( SELECT ";
  if ( this->firstSelect ) { qry << "*"; }
  else                     { qry << this->selects.str(); }
  qry << " FROM " << this->from;
  if ( ! this->firstWhere ) { qry << " WHERE " << this->wheres.str(); }
  if ( ! this->firstOrder ) { qry << " ORDER BY " << this->orders.str(); }
  qry << " )";
  return qry.str();
}


/* -------------------------------------------------------------------------- */

  std::unordered_set<std::string>
PkgQuery::filterSemvers(
  const std::unordered_set<std::string> & versions
) const
{
  static const std::vector<std::string> ignores = {
    "", "*", "any", "^*", "~*", "x", "X"
  };
  if ( ( ! this->semver.has_value() ) ||
       ( std::find( ignores.begin(), ignores.end(), * this->semver )
         != ignores.end()
       )
     )
    {
      return versions;
    }
  std::list<std::string>          args( versions.begin(), versions.end() );
  std::unordered_set<std::string> rsl;
  for ( auto & version : versions::semverSat( * this->semver, args ) )
    {
      rsl.emplace( std::move( version ) );
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<sqlite3pp::query>
PkgQuery::bind( sqlite3pp::database & db ) const
{
  std::shared_ptr<sqlite3pp::query> qry =
    std::make_shared<sqlite3pp::query>( db, this->str().c_str() );
  for ( const auto & [var, val] : this->binds )
    {
      qry->bind( var.c_str(), val, sqlite3pp::copy );
    }
  return qry;
}


/* -------------------------------------------------------------------------- */

  std::vector<row_id>
PkgQuery::execute( sqlite3pp::database & db ) const
{
  std::shared_ptr<sqlite3pp::query> qry = this->bind( db );
  std::vector<row_id>               rsl;

  /* If we don't need to handle `semver' this is easy. */
  if ( ! this->semver.has_value() )
    {
      for ( const auto & row : * qry )
        {
          rsl.push_back( row.get<long long>( 0 ) );
        }
      return rsl;
    }

  /* We can handle quite a bit of filtering and ordering in SQL, but `semver`
   * has to be handled with post-processing here. */

  std::unordered_set<std::string> versions;
  /* Use a vector to preserve ordering original ordering. */
  std::vector<std::pair<row_id, std::string>> idVersions;
  for ( const auto & row : * qry )
    {
      const auto & [_, version] = idVersions.emplace_back(
        std::make_pair( row.get<long long>( 0 ), row.get<std::string>( 1 ) )
      );
      versions.emplace( version );
    }
  versions = this->filterSemvers( versions );
  /* Filter SQL results to be those in the satisfactory list. */
  for ( const auto & elem : idVersions )
    {
      if ( versions.find( elem.second ) != versions.end() )
        {
          rsl.push_back( elem.first );
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

// TODO: remove after updating `getPackages' and test cases.
  std::pair<std::string, SQLBinds>
buildPkgQuery( const PkgQueryArgs & params, bool allFields )
{
  if ( allFields )
    {
      PkgQuery qry( params, std::vector<std::string> { "*" } );
      return std::make_pair( qry.str(), std::move( qry.binds ) );
    }
  else
    {
      PkgQuery qry( params );
      return std::make_pair( qry.str(), std::move( qry.binds ) );
    }
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox::pkgdb' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
