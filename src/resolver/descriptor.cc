/* ========================================================================== *
 *
 * @file resolver/descriptor.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

ManifestDescriptor::ManifestDescriptor( const ManifestDescriptorRaw & raw )
  : name( raw.name )
  , version( raw.version )
  , optional( raw.optional )
  , group( raw.packageGroup )
{
  /* You have to split `absPath' first. */
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

      this->subtree = Subtree( glob.front().value() );

      if ( raw.stability.has_value() && ( glob.front().value() != "catalog" ) )
        {
          throw std::runtime_error(
            "`stability' cannot be used with non-catalog paths"
          );
        }

      if ( glob.front().value() == "catalog" )
        {
          if ( glob.size() < 4 )
            {
              throw std::runtime_error(
                "`absPath' must have at least four parts for catalog paths"
              );
            }
          this->stability = glob.at( 2 ).value();
          this->path      = AttrPath {};
          for ( auto itr = glob.begin() + 3; itr != glob.end(); ++itr )
            {
              this->path->emplace_back( itr->value() );
            }
        }
      else
        {
          this->path = AttrPath {};
          for ( auto itr = glob.begin() + 2; itr != glob.end(); ++itr )
            {
              this->path->emplace_back( itr->value() );
            }
        }

      if ( glob.at( 1 ).has_value() )
        {
          this->systems = std::vector<std::string> { * glob.at( 1 ) };
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

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
