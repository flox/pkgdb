/* ========================================================================== *
 *
 * @file resolver/descriptor.cc
 *
 * @brief A set of user inputs used to set input preferences and query
 *        parameters during resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#include <regex>

#include "flox/core/exceptions.hh"
#include "flox/resolver/descriptor.hh"
#include "versions.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Sets either `version` or `semver` on
 *        a `flox::resolver::ManifestDescriptor`.
 *
 * Distinguishes between semver ranges and exact version matchers.
 * @param desc The descriptor to initialize.
 * @param version The version description to parse.
 */
static void
initManifestDescriptorVersion( ManifestDescriptor & desc,
                               const std::string &  version )
{
  /* Strip leading/trailing whitespace. */
  std::string trimmed = trim_copy( version );

  /* Empty is recognized as a glob/_any_ range. */
  if ( trimmed.empty() )
    {
      desc.semver = std::move( trimmed );
      return;
    }

  /* Try a quick detection based on first character.
   * We identify `=` as an explicit _exact version_ match. */
  switch ( trimmed.at( 0 ) )
    {
      case '=': desc.version = std::move( trimmed.substr( 1 ) ); break;

      case '*':
      case '~':
      case '^':
      case '>':
      case '<': desc.semver = std::move( trimmed ); break;

      default:
        /* If it's a valid semver or a date then it's not a range. */
        if ( versions::isSemver( trimmed ) || versions::isDate( trimmed )
             || ( ! versions::isSemverRange( trimmed ) ) )
          {
            desc.version = std::move( trimmed );
          }
        else /* Otherwise, assume a range. */
          {
            desc.semver = std::move( trimmed );
          }
        break;
    }
}


/* -------------------------------------------------------------------------- */

/** @brief Get a `flox::resolver::AttrPathGlob` from a string if necessary. */
static AttrPathGlob
maybeSplitAttrPathGlob( const ManifestDescriptorRaw::AbsPath & absPath )
{
  if ( std::holds_alternative<AttrPathGlob>( absPath ) )
    {
      return std::get<AttrPathGlob>( absPath );
    }
  AttrPathGlob   glob;
  flox::AttrPath path = splitAttrPath( std::get<std::string>( absPath ) );
  size_t         idx  = 0;
  for ( const auto & part : path )
    {
      /* Treat `null' or `*' as a glob. */
      /* TODO we verify that only the second option is a glob elsewhere, but we
       * could do that here instead */
      if ( ( ( part == "null" ) || ( part == "*" ) ) )
        {
          glob.emplace_back( std::nullopt );
        }
      else { glob.emplace_back( part ); }
      ++idx;
    }
  return glob;
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Sets various fields on a `flox::resolver::ManifestDescriptor`
 *        based on the `absPath` field.
 * @param desc The descriptor to initialize.
 * @param raw The raw description to parse.
 */
static void
initManifestDescriptorAbsPath( ManifestDescriptor &          desc,
                               const ManifestDescriptorRaw & raw )
{
  if ( ! raw.absPath.has_value() )
    {
      throw FloxException(
        "`absPath' must be set when calling "
        "`flox::resolver::ManifestDescriptor::initManifestDescriptorAbsPath'" );
    }

  /* You might need to parse a globbed attr path, so handle that first. */
  AttrPathGlob glob = maybeSplitAttrPathGlob( *raw.absPath );

  if ( glob.size() < 3 )
    {
      throw FloxException( "`absPath' must have at least three parts" );
    }

  const auto & first = glob.front();
  if ( ! first.has_value() )
    {
      throw FloxException(
        "`absPath' may only have a glob as its second element" );
    }
  desc.subtree = Subtree( *first );

  if ( raw.stability.has_value() && ( first.value() != "catalog" ) )
    {
      throw FloxException(
        "`stability' cannot be used with non-catalog paths" );
    }

  if ( first.value() == "catalog" )
    {
      if ( glob.size() < 4 )
        {
          throw FloxException(
            "`absPath' must have at least four parts for catalog paths" );
        }
      const auto & third = glob.at( 2 );
      if ( ! third.has_value() )
        {
          throw FloxException(
            "`absPath' may only have a glob as its second element" );
        }
      desc.stability = *third;
      desc.path      = AttrPath {};
      for ( auto itr = glob.begin() + 3; itr != glob.end(); ++itr )
        {
          const auto & elem = *itr;
          if ( ! elem.has_value() )
            {
              throw FloxException(
                "`absPath' may only have a glob as its second element" );
            }
          desc.path->emplace_back( *elem );
        }
    }
  else
    {
      desc.path = AttrPath {};
      for ( auto itr = glob.begin() + 2; itr != glob.end(); ++itr )
        {
          const auto & elem = *itr;
          if ( ! elem.has_value() )
            {
              throw FloxException(
                "`absPath' may only have a glob as its second element" );
            }
          desc.path->emplace_back( *elem );
        }
    }

  const auto & second = glob.at( 1 );
  if ( second.has_value() && ( ( *second ) != "null" )
       && ( ( *second ) != "*" ) )
    {
      desc.systems = std::vector<std::string> { *second };
      if ( raw.systems.has_value() && ( *raw.systems != *desc.systems ) )
        {
          throw FloxException(
            "`systems' list conflicts with `absPath' system specification" );
        }
    }
}


/* -------------------------------------------------------------------------- */

ManifestDescriptor::ManifestDescriptor( const ManifestDescriptorRaw & raw )
  : name( raw.name ), optional( raw.optional ), group( raw.packageGroup )
{
  /* Determine if `version' was a range or not.
   * NOTE: The string "4.2.0" is not a range, but "4.2" is!
   *       If you want to explicitly match the `version` field with "4.2" then
   *       you need to use "=4.2". */
  if ( raw.version.has_value() )
    {
      initManifestDescriptorVersion( *this, *raw.version );
    }

  /* You have to split `absPath' before doing most other fields. */
  if ( raw.absPath.has_value() )
    {
      initManifestDescriptorAbsPath( *this, raw );
    }
  else if ( raw.stability.has_value() )
    {
      this->subtree   = Subtree( "catalog" );
      this->stability = raw.stability;
    }

  /* Only set if it wasn't handled by `absPath`. */
  if ( ( ! this->systems.has_value() ) && raw.systems.has_value() )
    {
      this->systems = *raw.systems;
    }

  if ( raw.path.has_value() )
    {
      /* Split relative path */
      flox::AttrPath path;
      if ( std::holds_alternative<std::string>( *raw.path ) )
        {
          path = splitAttrPath( std::get<std::string>( *raw.path ) );
        }
      else { path = std::get<AttrPath>( *raw.path ); }

      if ( this->path.has_value() )
        {
          if ( this->path != path )
            {
              throw FloxException( "`path' conflicts with with `absPath'" );
            }
        }
      else { this->path = path; }
    }

  if ( raw.packageRepository.has_value() )
    {
      if ( raw.input.has_value() )
        {
          throw FloxException(
            "`packageRepository' may not be used with `input'" );
        }

      if ( std::holds_alternative<std::string>( *raw.packageRepository ) )
        {
          this->input
            = parseFlakeRef( std::get<std::string>( *raw.packageRepository ) );
        }
      else
        {
          this->input = nix::FlakeRef::fromAttrs(
            std::get<nix::fetchers::Attrs>( *raw.packageRepository ) );
        }
      assert( std::holds_alternative<nix::FlakeRef>( *this->input ) );
    }
  else if ( raw.input.has_value() ) { this->input = *raw.input; }
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
  /* Must exactly match either `pname' or `pkgAttrName'. */
  if ( this->name.has_value() ) { pqa.pnameOrPkgAttrName = *this->name; }

  if ( this->version.has_value() ) { pqa.version = *this->version; }
  else if ( this->semver.has_value() )
    {
      pqa.semver = *this->semver;
      /* Use `preferPreRelease' on `~<VERSION>-<TAG>' ranges. */
      if ( this->semver->at( 0 ) == '~' )
        {
          pqa.preferPreReleases = std::regex_match(
            *this->semver,
            std::regex( "~[^ ]+-.*", std::regex::ECMAScript ) );
        }
    }

  if ( this->subtree.has_value() )
    {
      pqa.subtrees = std::vector<Subtree> { *this->subtree };
    }

  if ( this->systems.has_value() ) { pqa.systems = *this->systems; }

  if ( this->stability.has_value() )
    {
      pqa.stabilities = std::vector<std::string> { *this->stability };
    }

  pqa.relPath = this->path;

  return pqa;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
