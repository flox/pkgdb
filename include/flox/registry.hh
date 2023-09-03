/* ========================================================================== *
 *
 * @file flox/registry.hh
 *
 * @brief A set of user inputs used to set input preferences during search
 *        and resolution.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <map>

#include <nlohmann/json.hpp>
#include <nix/flake/flakeref.hh>
#include <nix/fetchers.hh>

#include "flox/core/exceptions.hh"
#include "flox/core/types.hh"
#include "flox/core/util.hh"
#include "flox/pkgdb/pkg-query.hh"
#include "flox/flox-flake.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** Preferences associated with a registry input. */
struct InputPreferences {

  /**
   * Ordered list of subtrees to be searched.
   * Results will be grouped by subtree in the order they appear here.
   */
  std::optional<std::vector<subtree_type>> subtrees;

  /**
   * Ordered list of stabilities to be searched.
   * Catalog results will be grouped by stability in the order they
   * appear here.
   */
  std::optional<std::vector<std::string>> stabilities;

};  /* End struct `InputPreferences' */

void from_json( const nlohmann::json & jfrom,       InputPreferences & prefs );
void to_json(         nlohmann::json & jto,   const InputPreferences & prefs );


  template <typename T>
concept input_preferences_typename =
    std::is_base_of<InputPreferences, T>::value;


/* -------------------------------------------------------------------------- */

/** Preferences associated with a named registry input. */
struct RegistryInput : public InputPreferences {
  std::shared_ptr<nix::FlakeRef> from;
};  /* End struct `RegistryInput' */

void from_json( const nlohmann::json & jfrom,       RegistryInput & rip );
void to_json(         nlohmann::json & jto,   const RegistryInput & rip );

  template<typename T>
concept constructibe_from_registry_input =
    std::constructible_from<T, const RegistryInput &>;


/* -------------------------------------------------------------------------- */

/**
 * @brief A set of user inputs used to set input preferences during search
 *        and resolution.
 *
 * @example
 * ```
 * {
 *   "inputs": {
 *     "nixpkgs": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "NixOS"
 *       , "repo": "nixpkgs"
 *       }
 *     , "subtrees": ["legacyPackages"]
 *     }
 *   , "floco": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "aakropotkin"
 *       , "repo": "floco"
 *       }
 *     , "subtrees": ["packages"]
 *     }
 *   , "floxpkgs": {
 *       "from": {
 *         "type": "github"
 *       , "owner": "flox"
 *       , "repo": "floxpkgs"
 *       }
 *     , "subtrees": ["catalog"]
 *     , "stabilities": ["stable"]
 *     }
 *   }
 * , "defaults": {
 *     "subtrees": null
 *   , "stabilities": ["stable"]
 *   }
 * , "priority": ["nixpkgs", "floco", "floxpkgs"]
 * }
 * ```
 */
struct RegistryRaw {

  virtual ~RegistryRaw() = default;
  RegistryRaw( const RegistryRaw &  ) = default;
  RegistryRaw(       RegistryRaw && ) = default;

  RegistryRaw & operator=( const RegistryRaw &  ) = default;
  RegistryRaw & operator=(       RegistryRaw && ) = default;


  /** Settings and fetcher information associated with named inputs. */
  std::map<std::string, RegistryInput> inputs;

  /** Default/fallback settings for inputs. */
  InputPreferences defaults;

  /**
   * Priority order used to process inputs.
   * Inputs which do not appear in this list are handled in lexicographical
   * order after any explicitly named inputs.
   */
  std::vector<std::string> priority;

  /**
   * @a brief Return an ordered list of input names.
   *
   * This appends @a priority with any missing @a inputs in
   * lexicographical order.
   *
   * The resulting list contains wrapped references and need to be accessed
   * using @a std::reference_wrapper<T>::get().
   *
   * @example
   * ```
   * Registry reg = R"( {
   *   "inputs": {
   *     "floco": {
   *       "from": { "type": "github", "owner": "aakropotkin", "repo": "floco" }
   *     }
   *   , "floxpkgs": {
   *       "from": { "type": "github", "owner": "flox", "repo": "floxpkgs" }
   *     }
   *   , "nixpkgs": {
   *       "from": { "type": "github", "owner": "NixOS", "repo": "nixpkgs" }
   *     }
   *   }
   * , "priority": ["nixpkgs", "floxpkgs"]
   * } )"_json;
   * for ( const auto & name : reg.getOrder() )
   *   {
   *     std::cout << name.get() << " ";
   *   }
   * std::cout << std::endl;
   * // => nixpkgs floxpkgs floco
   * ```
   *
   * @return A list of input names in order of priority.
   */
    [[nodiscard]]
    virtual std::vector<std::reference_wrapper<const std::string>>
  getOrder() const;

  /** Reset to default state. */
    inline void
  clear()
  {
    this->inputs.clear();
    this->priority.clear();
  }

};  /* End struct `Registry' */


void from_json( const nlohmann::json & jfrom,       RegistryRaw & reg );
void to_json(         nlohmann::json & jto,   const RegistryRaw & reg );


/* -------------------------------------------------------------------------- */

/**
 * An input registry that may hold arbitrary types of inputs.
 * Unlike @a RegistryRaw, inputs are held in order, and any default settings
 * have been applied to inputs.
 */
  template<constructibe_from_registry_input InputType>
  requires input_preferences_typename<InputType>
class Registry : FloxFlakeParserMixin
{

  private:

    /**
     * Orginal raw registry.
     * This is saved to allow the raw user input to be recorded in lockfiles.
     */
    RegistryRaw registryRaw;

    /** A list of `<SHORTNAME>, <FLAKE>` pairs in priority order. */
    std::vector<std::pair<std::string, std::shared_ptr<InputType>>> inputs;


  public:

    using input_type = InputType;

    explicit Registry( const RegistryRaw & registry )
      : registryRaw( registry )
    {
      for ( const std::reference_wrapper<const std::string> & _name :
              registry.getOrder()
          )
        {
          const auto & pair = std::find_if(
            registry.inputs.begin()
          , registry.inputs.end()
          , [&]( const auto & pair ) { return pair.first == _name.get(); }
          );

          /* Fill default/fallback values if none are defined. */
          RegistryInput input = pair->second;
          if ( ! input.subtrees.has_value() )
            {
              input.subtrees = registry.defaults.subtrees;
            }
          if ( ! input.stabilities.has_value() )
            {
              input.stabilities = registry.defaults.stabilities;
            }
          this->inputs.emplace_back(
            std::make_pair( pair->first, std::make_shared<InputType>( input ) )
          );
        }
    }


    /**
     * Get a flake by name with convenience wrappers for evaluation.
     * @param name Registry shortname for the target flake.
     * @return A flake wrapped with a nix evaluator, or `nullptr` if no such
     *         @a name exists in the registry.
     */
      [[nodiscard]]
      std::shared_ptr<InputType>
    get( const std::string & name ) noexcept
    {
      auto maybeInput = std::find_if(
        this->inputs.begin()
      , this->inputs.end()
      , [&]( const auto & pair ) { return pair.first == name; }
      );
      if ( maybeInput == this->inputs.end() ) { return nullptr; }
      return maybeInput->second;
    }

    /**
     * Get a flake by name with convenience wrappers for evaluation.
     * @param name Registry shortname for the target flake.
     * @return A flake wrapped with a nix evaluator, or `nullptr` if no such
     *         @a name exists in the registry.
     */
      [[nodiscard]]
      const std::shared_ptr<InputType>
    get( const std::string & name ) const noexcept
    {
      const auto maybeInput = std::find_if(
        this->inputs.begin()
      , this->inputs.end()
      , [&]( const auto & pair ) { return pair.first == name; }
      );
      if ( maybeInput == this->inputs.end() ) { return nullptr; }
      return maybeInput->second;
    }


    [[nodiscard]]
      std::shared_ptr<InputType>
    at( const std::string & name )
    {
      std::shared_ptr<InputType> maybeInput = this->get( name );
      if ( maybeInput == nullptr )
        {
          throw std::out_of_range( "No such input '" + name + "'" );
        }
      return maybeInput;
    }

    [[nodiscard]]
      const std::shared_ptr<InputType>
    at( const std::string & name ) const
    {
      const std::shared_ptr<InputType> maybeInput = this->get( name );
      if ( maybeInput == nullptr )
        {
          throw std::out_of_range( "No such input '" + name + "'" );
        }
      return maybeInput;
    }


      [[nodiscard]]
      std::vector<std::reference_wrapper<const std::string>>
    getOrder() const
    {
      std::vector<std::reference_wrapper<const std::string>> order;
      /* Extract names from `flakes' list. */
      for ( const auto & pair : this->inputs )
        {
          order.emplace_back( pair.first );
        }
      return order;
    }


    [[nodiscard]]
    const RegistryRaw & getRaw() const { return this->registryRaw; }

    /* Forwarded container functions. */
    [[nodiscard]] auto begin()        { return this->inputs.begin();  }
    [[nodiscard]] auto end()          { return this->inputs.end();    }
    [[nodiscard]] auto begin()  const { return this->inputs.cbegin(); }
    [[nodiscard]] auto end()    const { return this->inputs.cend();   }
    [[nodiscard]] auto cbegin()       { return this->inputs.cbegin(); }
    [[nodiscard]] auto cend()         { return this->inputs.cend();   }
    [[nodiscard]] auto size()   const { return this->inputs.size();   }
    [[nodiscard]] auto empty()  const { return this->inputs.empty();  }

};  /* End class `Registry' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
