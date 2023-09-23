/* ========================================================================== *
 *
 * @file flox/resolver/manifest.hh
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <unordered_map>

#include <nlohmann/json.hpp>

#include "flox/pkgdb/input.hh"
#include "flox/resolver/state.hh"
#include "flox/resolver/descriptor.hh"
#include "flox/registry.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

struct ManifestRaw {

  // TODO: envBase

  // TODO: options
  struct Options {
    std::optional<std::vector<std::string>> systems;
  };  /* End struct `Options' */
  std::optional<Options> options;

  std::unordered_map<std::string, ManifestDescriptor> install;

  RegistryRaw registry;

  // TODO: vars
  // std::unordered_map<std::string, std::string> vars;

  // TODO: hook


};  /* End struct `ManifestRaw' */


/* -------------------------------------------------------------------------- */

/** @brief Convert a JSON object to a @a flox::resolver::ManifestRaw. */
void from_json( const nlohmann::json & jfrom, ManifestRaw & manifest );

// TODO:
/** @brief Convert a @a flox::resolver::ManifestRaw to a JSON object. */
//void to_json( nlohmann::json & jto, const ManifestRaw & manifest );


/* -------------------------------------------------------------------------- */

/** @brief Constructs @a pkgdb::PkgDbInput from manifest `registry` and
 *         `install.*.packageRepository` inputs.
 */
class ManifestInputFactory {

  private:

    nix::ref<nix::Store>  store;    /**< `nix` store connection. */
    std::filesystem::path cacheDir; /**< Cache directory. */


  public:

    using input_type = pkgdb::PkgDbInput;

    /** @brief Construct a factory using a `nix` evaluator. */
    explicit ManifestInputFactory(
      nix::ref<nix::Store>  & store
    , std::filesystem::path   cacheDir = pkgdb::getPkgDbCachedir()
    ) : store( store )
      , cacheDir( std::move( cacheDir ) )
    {}


    /**
     * @brief Construct an input from a @a RegistryInput.
     *
     * If @a name has the prefix "__inline__" the name is NOT passed through to
     * the @a PkgDbInput constructor.
     * This causes any resulting output to use the _flake reference_
     * URL instead.
     */
      [[nodiscard]]
      std::shared_ptr<pkgdb::PkgDbInput>
    mkInput( const std::string & name, const RegistryInput & input );


};  /* End class `ManifestInputFactory' */


static_assert( registry_input_factory<ManifestInputFactory> );


/* -------------------------------------------------------------------------- */

class Manifest : public pkgdb::PkgDbRegistryMixin<ManifestInputFactory> {

  protected:

    /* From `PkgDbRegistryMixin':
     *   std::shared_ptr<nix::Store>                         store
     *   std::shared_ptr<nix::EvalState>                     state
     *   bool                                                force    = false;
     *   std::shared_ptr<Registry<pkgdb::PkgDbInputFactory>> registry;
     */

    /** The original _raw_ manifest. */
    ManifestRaw raw;

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

    /* Cached list of systems collected from @a raw */
    std::vector<std::string> systems;


    /**
     * @brief Get a _base_ set of query arguments for the input associated with
     *        @a name and declared @a preferences.
     */
    [[nodiscard]]
    pkgdb::PkgQueryArgs getPkgQueryArgs( const std::string & name );


  private:

    /** @brief Collects _inline_ inputs from descriptors. */
    std::unordered_map<std::string, nix::FlakeRef> getInlineInputs() const;


    /**
     * @brief Collects _inline_ inputs from descriptors extending those
     *        declared in @a raw.registry.
     */
    void initRegistryRaw();


    [[nodiscard]] virtual std::vector<std::string> & getSystems() override;


  public:

    Manifest() = default;

    explicit Manifest( const ManifestRaw & raw ) : raw( raw ) {}


    /** @brief Get the _raw_ registry declaration. */
    [[nodiscard]] virtual RegistryRaw getRegistryRaw() override;


    [[nodiscard]]
    std::unordered_map<std::string, nix::FlakeRef> getLockedInputs();


};  /* End struct `Manifest' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
