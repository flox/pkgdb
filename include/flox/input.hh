/* ========================================================================== *
 *
 * @file flox/input.hh
 *
 * @brief Declares objects used to process _flake reference_ URI inputs.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <list>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include "flox/types.hh"
#include "flox/exceptions.hh"
#include "flox/package-set.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/** A single _flake reference_ input to be resolved, locked, and scraped. */
class Input {

  protected:

    /**
     * Locked flake reference containing the original, resolved, and locked
     * forms of the reference provided by `nix'.
     */
    std::shared_ptr<nix::flake::LockedFlake> _lockedFlake;


/* -------------------------------------------------------------------------- */

  public:

    /** Constructs an @a Input from a _flake reference_ URI string. */
    Input( std::string_view refURI );
    /** Constructs an @a Input from a parsed _flake reference_ object. */
    Input( const resolve::FloxFlakeRef & ref );


/* -------------------------------------------------------------------------- */

    /**
     * @return A unique _fingerprint_ hash associated with the input's
     *         locked flake.
     */
      nix::Hash
    getFingerprint() const
    {
      return this->_lockedFlake->getFingerprint();
    }


/* -------------------------------------------------------------------------- */

    /**
     * @return The set of defined flake output subtrees
     *         ( "packages", "legacyPackages", "catalog" ) in the input's flake.
     */
    virtual std::unordered_set<resolve::subtree_type> getSubtrees() = 0;

    /**
     * @param subtree The subtree ( @a resolve::subtree_type enum ) to
     *                check for.
     * @return `true` iff the input's flake outputs an attribute set
     *         for @a subtree.
     */
      virtual bool
    hasSubtree( const resolve::subtree_type & subtree )
    {
      const auto sts = this->getSubtrees();
      return sts.find( subtree ) != sts.cend();
    }

    /**
     * @param subtree The subtree ( string ) to check for.
     * @return `true` iff the input's flake outputs an attribute set
     *         for @a subtree.
     */
      virtual bool
    hasSubtree( std::string_view subtree )
    {
      return this->hasSubtree( resolve::parseSubtreeType( subtree ) );
    }


/* -------------------------------------------------------------------------- */

    /**
     * @param subtree Subtree enum.
     * @return The set of systems defined under flake output subtrees
     *         ( "x86_64-linux", "aarch64-darwin", etc ) in the input's flake.
     */
    virtual std::unordered_set<std::string_view> getSystems(
      const resolve::subtree_type & subtree
    ) = 0;

    /**
     * @param subtree Subtree name.
     * @return The set of systems defined under flake output @a subtree
     *         ( "x86_64-linux", "aarch64-darwin", etc ) in the input's flake.
     */
      virtual std::unordered_set<std::string_view>
    getSystems( std::string_view subtree )
    {
      return this->getSystems( resolve::parseSubtreeType( subtree ) );
    }

    /**
     * @return `true` iff @a subtree outputs an attribute set under @a system.
     */
      virtual bool
    hasSystem( const resolve::subtree_type & subtree, std::string_view system )
    {
      if ( ! this->hasSubtree( subtree ) ) { return false; }
      const auto ss = this->getSystems( subtree );
      return ss.find( system ) != ss.cend();
    }

    /**
     * @return `true` iff @a subtree outputs an attribute set under @a system.
     */
      virtual bool
    hasSystem( std::string_view subtree, std::string_view system )
    {
      return this->hasSystem( resolve::parseSubtreeType( subtree ), system );
    }


/* -------------------------------------------------------------------------- */

    /**
     * @return The set of stabilities ( if any ) output by the `catalog` subtree
     *         of the input's flake.
     *         Flakes without a `catalog` subtree always return the empty set.
     */
      virtual std::unordered_set<std::string_view>
    getStabilities( std::string_view system )
    {
      return {};
    }

    /**
     * @return The set of stabilities ( if any ) output by the `catalog` subtree
     *         of the input's flake.
     *         Flakes without a `catalog` subtree always return the empty set.
     */
      virtual bool
    hasStability( std::string_view system, std::string_view stability )
    {
      if ( ! this->hasSystem( resolve::ST_CATALOG, system ) ) { return false; }
      const auto sts = this->getStabilities( system );
      return sts.find( stability ) != sts.cend();
    }


/* -------------------------------------------------------------------------- */

    /** @return A list of package sets output by the input's flake. */
    virtual std::list<nix::ref<resolve::PackageSet>> getPackageSets() = 0;

    /**
     * @param subtree Subtree enum to get from.
     * @param system  System pair such as "x86_64-linux" get.
     * @return A pointer to a package set if it is output by the input's flake,
     *         or `nullptr` if no such output exists.
     */
    virtual std::shared_ptr<resolve::PackageSet> getPackageSet(
      const resolve::subtree_type & subtree
    ,       std::string_view        system
    ) = 0;

    /**
     * @param subtreeOrSystem Subtree name ( "packages" or "legacyPackages" ) or
     *                        system pair such as "x86_64-linux" get if
     *                        attempting to get from the `catalog` subtree.
     * @param systemOrCatalog  If @a subtreeOrSystem was either "packages" or
     *                         "legacyPackages" then a system pair such as
     *                         "x86_64-linux" to get.
     *                         If @a subtreeOrSystem was a system pair then a
     *                        `flox` catalog _stability_ name
     *                        ( "stable", "staging", "unstable" ) to get from.
     * @return A pointer to a package set if it is output by the input's flake,
     *         or `nullptr` if no such output exists.
     */
      virtual std::shared_ptr<resolve::PackageSet>
    getPackageSet( std::string_view subtreeOrSystem
                 , std::string_view systemOrCatalog
                 )
    {
      if ( resolve::isPkgsSubtree( subtreeOrSystem ) )
        {
          return this->getPackageSet(
            resolve::parseSubtreeType( subtreeOrSystem )
          , systemOrCatalog
          );
        }
      else if ( this->hasStability( subtreeOrSystem, systemOrCatalog ) )
        {
          throw resolve::ResolverException(
            "Input::getPackageSet(): Cannot get catalog package sets with "
            "virtual implementation."
          );
        }
      else
        {
          return nullptr;
        }
    }


/* -------------------------------------------------------------------------- */

};  /* End class `Input' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
