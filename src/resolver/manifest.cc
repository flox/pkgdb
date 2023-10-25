/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <optional>

#include "flox/pkgdb/params.hh"
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

static ManifestRaw
readManifestFromPath( const std::filesystem::path & manifestPath )
{
  if ( ! std::filesystem::exists( manifestPath ) )
    {
      throw InvalidManifestFileException( "no such path: "
                                          + manifestPath.string() );
    }
  return readAndCoerceJSON( manifestPath );
}


/* -------------------------------------------------------------------------- */

class UnlockedManifest
{

private:

  std::filesystem::path manifestPath;
  ManifestRaw           manifestRaw;

  /** Lazily locked registry. */
  std::optional<RegistryRaw> lockedRegistry;

  /**
   * Lazily converted @a flox::pkgdb::PkgQueryArgs record.
   * This is used as a _base_ for individual descriptor queries.
   */
  std::optional<pkgdb::PkgQueryArgs> baseQueryArgs;


public:

  ~UnlockedManifest()                          = default;
  UnlockedManifest()                           = default;
  UnlockedManifest( const UnlockedManifest & ) = default;
  UnlockedManifest( UnlockedManifest && )      = default;

  UnlockedManifest( std::filesystem::path manifestPath, ManifestRaw raw )
    : manifestPath( std::move( manifestPath ) ), manifestRaw( std::move( raw ) )
  {}

  explicit UnlockedManifest( std::filesystem::path manifestPath )
    : manifestPath( std::move( manifestPath ) )
    , manifestRaw( readManifestFromPath( this->manifestPath ) )
  {}


  UnlockedManifest &
  operator=( const UnlockedManifest & )
    = default;

  UnlockedManifest &
  operator=( UnlockedManifest && )
    = default;


  [[nodiscard]] std::filesystem::path
  getManifestPath() const
  {
    return this->manifestPath;
  }

  [[nodiscard]] const ManifestRaw &
  getManifestRaw() const
  {
    return this->manifestRaw;
  }

  [[nodiscard]] const RegistryRaw &
  getRegistryRaw() const
  {
    return this->manifestRaw.registry;
  }

  [[nodiscard]] const RegistryRaw &
  getLockedRegistry()
  {
    if ( ! this->lockedRegistry.has_value() )
      {
        this->lockedRegistry = lockRegistry( this->getRegistryRaw() );
      }
    return *this->lockedRegistry;
  }

  [[nodiscard]] const pkgdb::PkgQueryArgs &
  getBaseQueryArgs()
  {
    if ( ! this->baseQueryArgs.has_value() )
      {
        pkgdb::PkgQueryArgs args;
        if ( this->manifestRaw.options.systems.has_value() )
          {
            args.systems = *this->manifestRaw.options.systems;
          }

        if ( this->manifestRaw.options.allow.has_value() )
          {
            if ( this->manifestRaw.options.allow->unfree.has_value() )
              {
                args.allowUnfree = *this->manifestRaw.options.allow->unfree;
              }
            if ( this->manifestRaw.options.allow->broken.has_value() )
              {
                args.allowBroken = *this->manifestRaw.options.allow->broken;
              }
            args.licenses = this->manifestRaw.options.allow->licenses;
          }

        if ( this->manifestRaw.options.semver.has_value()
             && this->manifestRaw.options.semver->preferPreReleases
                  .has_value() )
          {
            args.preferPreReleases
              = *this->manifestRaw.options.semver->preferPreReleases;
          }
        this->baseQueryArgs = std::make_optional( std::move( args ) );
      }
    return *this->baseQueryArgs;
  }


}; /* End class `UnlockedManifest' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
