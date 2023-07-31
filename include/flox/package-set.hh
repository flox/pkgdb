/* ========================================================================== *
 *
 * @file flox/package-set.hh
 *
 * @brief Abstract declaration of a package set.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <concepts>
#include <list>
#include <optional>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/**
 * Abstract representation of a package set containing derivation metadata.
 * This is used to provide various container like utilities across different
 * back-ends ( implementations ) to avoid repeated routines.
 */
class PackageSet {

  public:
    /**
     * PackageSet "type" represented as a simple string.
     * This is used for error messages and working with abstract @a PackageSet
     * references in generic utility functions.
     */
    virtual std::string_view getType() const = 0;

    /** @return the flake output "subtree" associated with the package set. */
    virtual subtree_type getSubtree() const = 0;

    /** @return the architecture/platform associated with the package set. */
    virtual std::string_view getSystem() const = 0;

    /**
     * @return For package sets in a `catalog` @a subtree, returns the
     *         associated `flox` _stability_ associated with the package set.
     *         For non-catalog package sets, returns @a std::nullopt.
     */
    virtual std::optional<std::string_view> getStability() const = 0;

    /**
     * @return The flake reference associated with the package set indicating
     *         its source.
     */
    virtual FloxFlakeRef getRef() const = 0;

    /**
     * Packages contained by @a this package set are referred to as being
     * "relative" to a `<SUBTREE>.<SYSTEM>[.<STABILITY>]` attribute path prefix.
     * @return an list of strings representing the attribute path to the root
     *         ( _prefix_ ) of the package set.
     */
      virtual std::list<std::string_view>
    getPrefix() const
    {
      auto s = this->getStability();
      if ( s.has_value() )
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          , s.value()
          };
        }
      else
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          };
        }
    }

    /** @return The number of packages in the package set. */
    virtual std::size_t size() = 0;

    /** @return true iff the package set has no packages. */
    virtual bool empty() { return this->size() <= 0; }

    /**
     * Predicate which checks to see if the package set has a package at the
     * relative path @a path.
     * @param path A relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return `true` iff the package set has a package at @a path.
     */
    virtual bool hasRelPath( const std::list<std::string_view> & path ) = 0;

    /**
     * Attempts to get package metadata associated with the relative path
     * @a path if it exists.
     * @param path A relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return `nullptr` if the package set does not contain a package at
     *         @a path, otherwise a pointer to the requested package metadata.
     */
    virtual std::shared_ptr<Package> maybeGetRelPath(
      const std::list<std::string_view> & path
    ) = 0;

    /**
     * Gets package metadata associated with the relative path @a path.
     * Throws an error if the package set is missing the requested metadata.
     * @param path A relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return A non-null pointer to the requested package metadata.
     */
      virtual nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path )
    {
      std::shared_ptr<Package> p = this->maybeGetRelPath( path );
      if ( p == nullptr )
        {
          std::string msg( "PackageSet::getRelPath(): No such path '" );
          msg += this->getRef().to_string();
          msg += "#";
          bool fst = true;
          for ( const auto & p : this->getPrefix() )
            {
              if ( fst ) { msg += p; fst = false; }
              else       { msg += p; msg += ".";  }
            }
          for ( const auto & p : path )
            {
              if ( p.find( '.' ) == p.npos )
                {
                  msg += '.';
                  msg += p;
                }
              else
                {
                  msg += ".\"";
                  msg += p;
                  msg += '"';
                }
            }
          msg += "'.";
          throw ResolverException( msg.c_str() );
        }
      return (nix::ref<Package>) p;
    }

};  /* End class `PackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
