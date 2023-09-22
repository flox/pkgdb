/* ========================================================================== *
 *
 * @file resolver/descriptor.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include "versions.hh"
#include "flox/resolver/descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/** @brief Distinguish between semver ranges and exact version matchers. */
  static void
initManifestDescriptorVersion(       ManifestDescriptor & desc
                             , const std::string        & version
                             )
{
  switch ( version.at( 0 ) )
    {
      case '=':
        desc.version = version.substr( 1 );
        break;

      case '*':
      case '~':
      case '^':
      case '>':
      case '<':
        desc.semver = version;
        break;

      default:
        /* If it's a valid semver, then it's not a range. */
        if ( versions::isSemver( version ) )
          {
            desc.version = version;
          }
        else /* Otherwise, assume a range. */
          {
            desc.semver = version;
          }
        break;
    }
}


/* -------------------------------------------------------------------------- */

ManifestDescriptor::ManifestDescriptor( const ManifestDescriptorRaw & raw )
  : name( raw.name )
  , optional( raw.optional )
  , group( raw.packageGroup )
{
  /* Determine if `version' was a range or not.
   * NOTE: The string "4.2.0" is not a range, but "4.2" is!
   *       If you want to explicitly match the `version` field with "4.2" then
   *       you need to use "=4.2". */
  if ( raw.version.has_value() )
    {
      initManifestDescriptorVersion( * this, * raw.version );
    }

  /* You have to split `absPath' before doing most other fields. */
  if ( raw.absPath.has_value() )
    {
      /* You might need to parse a globbed attr path, so handle that first. */
      AttrPathGlob glob;
      if ( std::holds_alternative<AttrPathGlob>( * raw.absPath ) )
        {
          glob = std::get<AttrPathGlob>( * raw.absPath );
        }
      else
        {
          flox::AttrPath path =
            splitAttrPath( std::get<std::string>( * raw.absPath ) );
          size_t idx = 0;
          for ( const auto & part : path )
            {
              /* Treat `null' or `*' in the second element as a glob. */
              if ( ( idx == 1 ) && ( ( part == "null" ) || ( part == "*" ) ) )
                {
                  glob.emplace_back( std::nullopt );
                }
              else
                {
                  glob.emplace_back( part );
                }
              ++idx;
            }
        }

      if ( glob.size() < 3 )
        {
          throw std::runtime_error(
            "`absPath' must have at least three parts"
          );
        }

      const auto & first = glob.front();
      if ( ! first.has_value() )
        {
          throw std::runtime_error(
            "`absPath' may only have a glob as its second element"
          );
        }
      this->subtree = Subtree( * first );

      if ( raw.stability.has_value() && ( first.value() != "catalog" ) )
        {
          throw std::runtime_error(
            "`stability' cannot be used with non-catalog paths"
          );
        }

      if ( first.value() == "catalog" )
        {
          if ( glob.size() < 4 )
            {
              throw std::runtime_error(
                "`absPath' must have at least four parts for catalog paths"
              );
            }
          const auto & third = glob.at( 2 );
          if ( ! third.has_value() )
            {
              throw std::runtime_error(
                "`absPath' may only have a glob as its second element"
              );
            }
          this->stability = * third;
          this->path      = AttrPath {};
          for ( auto itr = glob.begin() + 3; itr != glob.end(); ++itr )
            {
              const auto & elem = * itr;
              if ( ! elem.has_value() )
                {
                  throw std::runtime_error(
                    "`absPath' may only have a glob as its second element"
                  );
                }
              this->path->emplace_back( * elem );
            }
        }
      else
        {
          this->path = AttrPath {};
          for ( auto itr = glob.begin() + 2; itr != glob.end(); ++itr )
            {
              const auto & elem = * itr;
              if ( ! elem.has_value() )
                {
                  throw std::runtime_error(
                    "`absPath' may only have a glob as its second element"
                  );
                }
              this->path->emplace_back( * elem );
            }
        }

      const auto & second = glob.at( 1 );
      if ( second.has_value() )
        {
          this->systems = std::vector<std::string> { * second };
          if ( raw.systems.has_value() && ( * raw.systems != * this->systems ) )
            {
              throw std::runtime_error(
                "`systems' list conflicts with `absPath' system specification"
              );
            }
        }

    }
  else if ( raw.stability.has_value() )
    {
      this->subtree = Subtree( "catalog" );
      this->stability = raw.stability;
    }

  /* Only set if it wasn't handled by `absPath`. */
  if ( ( ! this->systems.has_value() ) && raw.systems.has_value() )
    {
      this->systems = * raw.systems;
    }

  if ( raw.path.has_value() )
    {
      /* Split relative path */
      flox::AttrPath path;
      if ( std::holds_alternative<std::string>( * raw.path ) )
        {
          path = splitAttrPath( std::get<std::string>( * raw.path ) );
        }
      else
        {
          path = std::get<AttrPath>( * raw.path );
        }

      if ( this->path.has_value() )
        {
          if ( this->path != path )
            {
              throw std::runtime_error(
                  "`path' conflicts with with `absPath'"
              );
            }
        }
      else
        {
          this->path = path;
        }
    }

  if ( raw.packageRepository.has_value() )
    {
      if ( raw.input.has_value() )
        {
          throw std::runtime_error(
            "`packageRepository' may not be used with `input'"
          );
        }

      if ( std::holds_alternative<std::string>( * raw.packageRepository ) )
        {
          this->input =
            parseFlakeRef( std::get<std::string>( * raw.packageRepository ) );
        }
      else
        {
          this->input = nix::FlakeRef::fromAttrs(
            std::get<nix::fetchers::Attrs>( * raw.packageRepository )
          );
        }
    }
  else if ( raw.input.has_value() )
    {
      this->input = * raw.input;
    }

}


/* -------------------------------------------------------------------------- */

  void
ManifestDescriptor::clear()
{
  this->name      = std::nullopt;
  this->optional  = false;
  this->group     = std::nullopt;
  this->version   = std::nullopt;
  this->semver    = std::nullopt;
  this->subtree   = std::nullopt;
  this->systems   = std::nullopt;
  this->stability = std::nullopt;
  this->path      = std::nullopt;
  this->input     = std::nullopt;
}


/* -------------------------------------------------------------------------- */

  pkgdb::PkgQueryArgs &
ManifestDescriptor::fillPkgQueryArgs( pkgdb::PkgQueryArgs & pqa ) const
{
  // TODO
  return pqa;
}


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
