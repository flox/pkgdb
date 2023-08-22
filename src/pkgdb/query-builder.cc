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

  std::pair<std::string, SQLBinds>
buildPkgQuery( const PkgQueryArgs & params )
{
  /* It's a pain to use `bind' with `in` lists because each element would need
   * its own variable, so instead we scan for unsafe characters and quote
   * /the good ol' fashioned way/ in those cases.
   * Other than that, we use `bind`. */

  using namespace sql;

  // TODO: validate params
  SelectModel q;
  q.select( "id" ).from( "v_PackagesSearch" );
  std::unordered_map<std::string, std::string> binds;

  if ( params.name.has_value() )
    {
      q.where( column( "name" ) == Param( ":name" ) );
      binds.emplace( ":name", params.name.value() );
    }

  if ( params.pname.has_value() )
    {
      q.where( column( "pname" ) == Param( ":pname" ) );
      binds.emplace( ":pname", params.pname.value() );
    }

  if ( params.version.has_value() )
    {
      q.where( column( "version" ) == Param( ":version" ) );
      binds.emplace( ":version", params.version.value() );
    }

  if ( params.licenses.has_value() && ( ! params.licenses.value().empty() ) )
    {
      for ( const auto & l : params.licenses.value() )
        {
          if ( l.find( '\'' ) != l.npos )
            {
              throw flox::pkgdb::PkgDbException(
                ""
              , "License '" + l + "' contains illegal character \"'\""
              );
            }
        }
      q.where( column( "license" ).in( params.licenses.value() ) );
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
      if ( params.subtrees.has_value() &&
           ( ( params.subtrees.value().size() != 1 ) ||
             ( params.subtrees.value().front() != ST_CATALOG )
           )
         )
        {
          throw flox::pkgdb::PkgDbException(
            ""
          , "Stability filters may only be used in `catalog' subtrees, "
            "but a non-catalog subtree was indicated."
          );
        }
      q.where( column( "subtree" ) == "catalog" );
    }
  else if ( params.subtrees.has_value() && params.subtrees.value().size() == 1 )
    {
      std::string s;
      switch ( params.subtrees.value().front() )
        {
          case ST_LEGACY:   s = "legacyPackages"; break;
          case ST_PACKAGES: s = "packages";       break;
          case ST_CATALOG:  s = "catalog";        break;
          default:
            throw flox::pkgdb::PkgDbException( "", "Invalid subtree" );
            break;
        }
      q.where( column( "subtree" ) == s );
    }
  else if ( params.subtrees.has_value() )
    {
      std::vector<std::string> quoted;
      for ( const auto s : params.subtrees.value() )
        {
          switch ( s )
            {
              case ST_LEGACY:
                quoted.emplace_back( "legacyPackages" );
                break;
              case ST_PACKAGES: quoted.emplace_back( "packages" ); break;
              case ST_CATALOG:  quoted.emplace_back( "catalog" );  break;
              default:
                throw flox::pkgdb::PkgDbException( "", "Invalid subtree" );
                break;
            }
        }
      q.where( column( "subtree" ).in( quoted ) );
    }

  /* Systems */
  if ( params.systems.size() == 1 )
    {
      if ( std::find( flox::defaultSystems.begin()
                    , flox::defaultSystems.end()
                    , params.systems.front()
                    )
           == flox::defaultSystems.end()
         )
        {
          throw flox::pkgdb::PkgDbException(
            ""
          , "Invalid system '" + params.systems.front() + "'"
          );
        }
      q.where( column( "system" ) == params.systems.front() );
    }
  else
    {
      for ( const auto & s : params.systems )
        {
          if ( std::find( flox::defaultSystems.begin()
                        , flox::defaultSystems.end()
                        , s
                        )
               == flox::defaultSystems.end()
             )
            {
              throw flox::pkgdb::PkgDbException(
                ""
              , "Invalid system '" + s + "'"
              );
            }
        }
      q.where( column( "system" ).in( params.systems ) );
    }

  /* Stabilities */
  if ( params.stabilities.has_value() )
    {
      if ( params.stabilities.value().size() == 1 )
        {
          if ( std::find( flox::defaultCatalogStabilities.begin()
                        , flox::defaultCatalogStabilities.end()
                        , params.stabilities.value().front()
                        )
               == flox::defaultCatalogStabilities.end()
             )
            {
              throw flox::pkgdb::PkgDbException(
                ""
              , "Invalid stability '" + params.stabilities.value().front() + "'"
              );
            }
          q.where(
            column( "stability" ) == params.stabilities.value().front()
          );
        }
      else
        {
          for ( const auto & s : params.stabilities.value() )
            {
              if ( std::find( flox::defaultCatalogStabilities.begin()
                            , flox::defaultCatalogStabilities.end()
                            , s
                            )
                   == flox::defaultCatalogStabilities.end()
                 )
                {
                  throw flox::pkgdb::PkgDbException(
                    ""
                  , "Invalid stability '" + s + "'"
                  );
                }
            }
          q.where( column( "stability" ).in( params.stabilities.value() ) );
        }
    }

  // TODO: match
  // TODO: semver and pre-releases

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
