/* ========================================================================== *
 *
 * @file flox/resolver/manifest.hh
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/input.hh"
#include "flox/registry.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/resolver/state.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/** @brief An exception thrown when the value of manifestPath is invalid */
class InvalidManifestFileException : public FloxException
{

private:

  static constexpr std::string_view categoryMsg
    = "invalid value for manifest file";

public:

  explicit InvalidManifestFileException( std::string_view contextMsg )
    : FloxException( contextMsg )
  {}

  [[nodiscard]] error_category
  getErrorCode() const noexcept override
  {
    return EC_INVALID_MANIFEST_FILE;
  }

  [[nodiscard]] std::string_view
  getCategoryMessage() const noexcept override
  {
    return this->categoryMsg;
  }


}; /* End class `InvalidManifestFileException' */


/* -------------------------------------------------------------------------- */

/**
 * @brief A _raw_ description of an environment to be read from a file.
 *
 * This _raw_ struct is defined to generate parsers, and its declarations simply
 * represent what is considered _valid_.
 * On its own, it performs no real work, other than to validate the input.
 */
struct ManifestRaw
{

  struct EnvBase
  {
    std::optional<std::string> floxhub;
    std::optional<std::string> dir;

    void
    check() const;
  }; /* End struct `EnvBase' */
  std::optional<EnvBase> envBase;

  struct Options
  {

    std::optional<std::vector<std::string>> systems;

    struct Allows
    {
      std::optional<bool>                     unfree;
      std::optional<bool>                     broken;
      std::optional<std::vector<std::string>> licenses;
    }; /* End struct `Allows' */
    std::optional<Allows> allow;

    struct Semver
    {
      std::optional<bool> preferPreReleases;
    }; /* End struct `Semver' */
    std::optional<Semver> semver;

    std::optional<std::string> packageGroupingStrategy;
    std::optional<std::string> activationStrategy;
    // TODO: Other options

  }; /* End struct `Options' */
  Options options;

  std::unordered_map<std::string, ManifestDescriptor> install;

  RegistryRaw registry;

  std::unordered_map<std::string, std::string> vars;

  struct Hook
  {
    std::optional<std::string> script;
    std::optional<std::string> file;

    /**
     * @brief Validate `Hook` fields, throwing an exception if its contents
     *        are invalid.
     */
    void
    check() const;
  }; /* End struct `ManifestRaw::Hook' */
  std::optional<Hook> hook;


}; /* End struct `ManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ManifestRaw. */
void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest );

// TODO:
/** @brief Convert a @a flox::resolver::ManifestRaw to a JSON object. */
// void to_json( nlohmann::json & jto, const ManifestRaw & manifest );


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
