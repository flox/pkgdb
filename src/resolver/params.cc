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
  this->stability         = std::nullopt;
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
      /* If path is absolute set `subtree', `systems', and `stabilities'. */
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

          if ( subtree == ST_CATALOG )
            {
              if ( this->path->size() < 4 )
                {
                  throw InvalidPkgDescriptorException(
                    "Absolute attribute paths in catalogs must have at "
                    "least four elements" );
                }
              pqa.stabilities
                = std::vector<std::string> { *this->path->at( 2 ) };
              fillRelPath( 3 );
            }
          else { fillRelPath( 2 ); }
          pqa.subtrees = std::vector<Subtree> { subtree };
        }
      else { fillRelPath( 0 ); }
      pqa.relPath = std::move( relPath );
    }

  return pqa;
}


/* -------------------------------------------------------------------------- */

argparse::Argument &
ManifestFileMixin::addManifestFileOption( argparse::ArgumentParser &parser )
{
  return parser.add_argument( "--manifest" )
    .help( "The path to the 'manifest.{toml,yaml,json}' file." )
    .metavar( "PATH" )
    .action( [&]( const std::string &strPath )
             { this->manifestPath = nix::absPath( strPath ); } );
}

argparse::Argument &
ManifestFileMixin::addManifestFileArg( argparse::ArgumentParser &parser,
                                       bool                      required )
{
  argparse::Argument &arg
    = parser.add_argument( "manifest" )
        .help( "The path to a 'manifest.{toml,yaml,json}' file." )
        .metavar( "MANIFEST-PATH" )
        .action( [&]( const std::string &strPath )
                 { this->manifestPath = nix::absPath( strPath ); } );
  return required ? arg.required() : arg;
}

std::filesystem::path
ManifestFileMixin::getManifestPath()
{
  if ( ! this->manifestPath.has_value() )
    {
      throw InvalidManifestFileException( "no manifest path given" );
    }
  return *this->manifestPath;
}

const RegistryRaw &
ManifestFileMixin::getRegistryRaw()
{
  if ( ! this->registryRaw.has_value() ) { this->loadContents(); }
  return *this->registryRaw;
}

void
ManifestFileMixin::loadContents()
{
  this->contents = readAndCoerceJSON( this->getManifestPath() );
  /* Fill `registryRaw' */
  try
    {
      this->contents.at( "registry" ).get_to( this->registryRaw );
    }
  catch ( const nlohmann::json::out_of_range & )
    {
      throw InvalidManifestFileException(
        "manifest does not contain a registry" );
    }
  catch ( const nlohmann::json::type_error & )
    {
      throw InvalidManifestFileException(
        "manifest does not contain a valid registry" );
    }
  catch ( const FloxException &err )
    {
      /* Pass the inner error as it was. */
      throw err;
    }
  catch ( const std::exception &err )
    {
      throw InvalidManifestFileException( "failed to load manifest registry: "
                                          + std::string( err.what() ) );
    }
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
