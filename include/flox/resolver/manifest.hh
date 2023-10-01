/* ========================================================================== *
 *
 * @file flox/resolver/manifest.hh
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <unordered_map>
#include <unordered_set>
#include <optional>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/input.hh"
#include "flox/resolver/state.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

struct ManifestRaw {

  struct EnvBase {
    std::optional<std::string> floxhub;
    std::optional<std::string> dir;
  };  /* End struct `EnvBase' */
  std::optional<EnvBase> envBase;

  struct Options {

    std::optional<std::vector<std::string>> systems;

    struct Allows {
      std::optional<bool>                     unfree;
      std::optional<bool>                     broken;
      std::optional<std::vector<std::string>> licenses;
    };  /* End struct `Allows' */
    std::optional<Allows> allow;

    struct Semver {
      std::optional<bool> preferPreReleases;
    };  /* End struct `Semver' */
    std::optional<Semver> semver;

    std::optional<std::string> packageGroupingStrategy;
    std::optional<std::string> activationStrategy;
    // TODO: Other options

  };  /* End struct `Options' */
  Options options;

  std::unordered_map<std::string, ManifestDescriptor> install;

  RegistryRaw registry;

  std::unordered_map<std::string, std::string> vars;

  struct Hook {
    std::optional<std::string> script;
    std::optional<std::string> file;
  };  /* End struct `Hook' */
  std::optional<Hook> hook;


};  /* End struct `ManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ManifestRaw. */
void from_json( const nlohmann::json & jfrom, ManifestRaw & manifest );

// TODO:
/** @brief Convert a @a flox::resolver::ManifestRaw to a JSON object. */
//void to_json( nlohmann::json & jto, const ManifestRaw & manifest );


/* -------------------------------------------------------------------------- */

class Manifest : public pkgdb::PkgDbRegistryMixin {

  protected:

    /* From `PkgDbRegistryMixin':
     *   std::shared_ptr<nix::Store>                         store;
     *   std::shared_ptr<nix::EvalState>                     state;
     *   bool                                                force    = false;
     *   std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;
     */

    /** The original _raw_ manifest. */
    ManifestRaw raw;

    pkgdb::QueryPreferences preferences;

    /**
     * Registry which includes undeclared _inline_ inputs.
     *
     * This a superset of @a raw.registry which adds any input declared _inline_
     * as a part of a descriptor.
     *
     * _Inline_ inputs carry the prefix `__inline__<ID>` where `ID` is the
     * descriptor `id` which declared the input.
     */
    std::optional<RegistryRaw> registryRaw;

    /**
     * @brief A map of groups to the IDs of descriptors which declare them.
     *
     * The reserved group "__ungrouped__" contains all descriptors that do not
     * declare a group.
     */
    std::unordered_map<std::string, std::unordered_set<std::string>> groups;

    /**
     * @brief Get a _base_ set of query arguments for the input associated with
     *        @a name and declared @a preferences.
     */
    [[nodiscard]]
    pkgdb::PkgQueryArgs getPkgQueryArgs( const std::string & name );


  private:

    /** @brief Initialize @a preferences from @a raw.options. */
    void initPreferences();

    /** @brief Initialize @a groups from @a raw.install. */
    void initGroups();

    // FIXME:
    public:

    /** @brief Collects _inline_ inputs from descriptors. */
    std::unordered_map<std::string, nix::FlakeRef> getInlineInputs() const;

    private:


    /**
     * @brief Collects _inline_ inputs from descriptors extending those
     *        declared in @a raw.registry.
     */
    void initRegistryRaw();


    [[nodiscard]] virtual std::vector<std::string> & getSystems() override;


  public:

    Manifest() = default;

    explicit Manifest( const ManifestRaw & raw ) : raw( raw )
    {
      this->initPreferences();
      this->initGroups();
    }


    /** @brief Get the _raw_ registry declaration. */
    [[nodiscard]] virtual RegistryRaw getRegistryRaw() override;


    /** @brief Get a list of all registry inputs' locked _flake references_. */
    [[nodiscard]]
    std::unordered_map<std::string, nix::FlakeRef> getLockedInputs();


    /** @brief Get a descriptor by its `id` ( key into `install` ). */
    [[nodiscard]]
    std::optional<ManifestDescriptor> getDescriptor( const std::string & iid );


    /** @brief Get query preferences. */
      [[nodiscard]]
      const pkgdb::QueryPreferences &
    getPreferences() const
    {
      return this->preferences;
    }

    /**
     * @brief Collect descriptors into declared groups.
     *
     * The reserved group "__ungrouped__" contains all descriptors that do not
     * declare a group.
     */
      [[nodiscard]]
      std::unordered_map<std::string, std::unordered_set<std::string>>
    getDescriptorGroups() const
    {
      return this->groups;
    }


    struct Resolved {
      std::string input;  /**< Input shortname or locked URL */
      AttrPath    path;   /**< Absolute attribute path */
    };

   /**
    * @brief NAIVE resolution routine.
    *
    * Ignores groups!
    *
    * TODO: Don't ignore groups.
    * TODO: Handle `system` grouping.
    */
     [[nodiscard]]
     std::vector<Resolved>
   resolveDescriptor( const std::string & iid );


};  /* End struct `Manifest' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
