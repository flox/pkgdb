/* ========================================================================== *
 *
 * @file resolve/params.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/globals.hh>

#include "flox/core/util.hh"
#include "flox/resolver/manifest.hh"
#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
PkgDescriptorRaw::clear()
{
  this->pkgdb::PkgDescriptorBase::clear();
  this->input             = std::nullopt;
  this->path              = std::nullopt;
  this->subtree           = std::nullopt;
  this->preferPreReleases = std::nullopt;
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json &jfrom, PkgDescriptorRaw &desc )
{
  pkgdb::from_json( jfrom, dynamic_cast<pkgdb::PkgDescriptorBase &>( desc ) );
  try
    {
      jfrom.at( "pnameOrPkgAttrName" ).get_to( desc.pnameOrPkgAttrName );
    }
  catch ( const nlohmann::json::out_of_range & )
    {}
  try
    {
      jfrom.at( "preferPreReleases" ).get_to( desc.preferPreReleases );
    }
  catch ( const nlohmann::json::out_of_range & )
    {}
  try
    {
      jfrom.at( "path" ).get_to( desc.path );
    }
  catch ( const nlohmann::json::out_of_range & )
    {}
}


void
to_json( nlohmann::json &jto, const PkgDescriptorRaw &desc )
{
  pkgdb::to_json( jto, dynamic_cast<const pkgdb::PkgDescriptorBase &>( desc ) );
  jto["pnameOrPkgAttrName"] = desc.pnameOrPkgAttrName;
  jto["preferPreReleases"]  = desc.preferPreReleases;
  jto["path"]               = desc.path;
}


/* -------------------------------------------------------------------------- */

pkgdb::PkgQueryArgs &
PkgDescriptorRaw::fillPkgQueryArgs( pkgdb::PkgQueryArgs &pqa ) const
{
  /* XXX: DOES NOT CLEAR FIRST! We are called after global preferences. */
  pqa.name    = this->name;
  pqa.pname   = this->pname;
  pqa.version = this->version;
  pqa.semver  = this->semver;

  /* Set pre-release preference. We get priority over _global_ preferences. */
  if ( this->preferPreReleases.has_value() )
    {
      pqa.preferPreReleases = *this->preferPreReleases;
    }

  /* Handle `path' parameter. */
  if ( this->path.has_value() && ( ! this->path->empty() ) )
    {
      flox::AttrPath relPath;
      auto           fillRelPath = [&]( int start )
      {
        std::transform( this->path->begin() + start,
                        this->path->end(),
                        std::back_inserter( relPath ),
                        []( const auto &s ) { return *s; } );
      };
      /* If path is absolute set `subtree' and `systems'. */
      if ( std::find( getDefaultSubtrees().begin(),
                      getDefaultSubtrees().end(),
                      this->path->front() )
           != getDefaultSubtrees().end() )
        {
          if ( this->path->size() < 3 )
            {
              throw InvalidPkgDescriptorException(
                "Absolute attribute paths must have at least three elements" );
            }
          Subtree subtree = Subtree::parseSubtree( *this->path->at( 0 ) );
          if ( this->path->at( 1 ).has_value() )
            {
              pqa.systems = std::vector<std::string> { *this->path->at( 1 ) };
            }

          fillRelPath( 2 );
          pqa.subtrees = std::vector<Subtree> { subtree };
        }
      else { fillRelPath( 0 ); }
      pqa.relPath = std::move( relPath );
    }

  return pqa;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver

/* -------------------------------------------------------------------------- */

/* Instantiate templates. */
namespace flox::pkgdb {
template struct QueryParams<resolver::PkgDescriptorRaw>;
}  // namespace flox::pkgdb


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
