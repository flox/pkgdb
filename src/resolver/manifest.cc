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

  /** Lazily converted @a flox::pkgdb::QueryPreferences record. */
  std::optional<pkgdb::QueryPreferences> preferences;


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

  [[nodiscard]] const pkgdb::QueryPreferences &
  getQueryPreferences()
  {
    if ( ! this->preferences.has_value() )
      {
        pkgdb::QueryPreferences prefs;
        if ( this->manifestRaw.options.systems.has_value() )
          {
            prefs.systems = *this->manifestRaw.options.systems;
          }

        if ( this->manifestRaw.options.allow.has_value() )
          {
            if ( this->manifestRaw.options.allow->unfree.has_value() )
              {
                prefs.allow.unfree = *this->manifestRaw.options.allow->unfree;
              }
            if ( this->manifestRaw.options.allow->broken.has_value() )
              {
                prefs.allow.broken = *this->manifestRaw.options.allow->broken;
              }
            if ( this->manifestRaw.options.allow->licenses.has_value() )
              {
                prefs.allow.licenses
                  = *this->manifestRaw.options.allow->licenses;
              }
          }

        if ( this->manifestRaw.options.semver.has_value() )
          {
            if ( this->manifestRaw.options.semver->preferPreReleases
                   .has_value() )
              {
                prefs.semver.preferPreReleases
                  = *this->manifestRaw.options.semver->preferPreReleases;
              }
          }
        this->preferences = std::make_optional( std::move( prefs ) );
      }

    return *this->preferences;
  }


}; /* End class `UnlockedManifest' */


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
