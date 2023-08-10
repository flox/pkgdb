/* ========================================================================== *
 *
 * @file flox/flake-package.hh
 *
 * @brief Provides a @a flox::Package implementation which are pulled from
 *        evaluation of a `nix` flake.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/names.hh>

#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  /* Forward declare a friend. */
  namespace pkgdb { class PkgDb; }

/* -------------------------------------------------------------------------- */

/** /
 * A @a flox::Package implementation which are pulled from evaluation of a
 * `nix` flake.
 */
class FlakePackage : public Package {

  public:
    friend class pkgdb::PkgDb;

  private:
    Cursor                   _cursor;
    std::vector<std::string> _pathS;

    bool _hasMetaAttr    = false;
    bool _hasPnameAttr   = false;
    bool _hasVersionAttr = false;

    std::string                _fullName;
    std::string                _pname;
    std::string                _version;
    std::optional<std::string> _semver;
    std::string                _system;
    subtree_type               _subtree;
    std::optional<std::string> _license;

    void init( bool checkDrv = true );


/* -------------------------------------------------------------------------- */

  public:

    FlakePackage(       Cursor                     cursor
                , const std::vector<std::string> & path
                ,       bool                       checkDrv = true
                )
      : _cursor( cursor )
      , _pathS( path )
      , _fullName( cursor->getAttr( "name" )->getString() )
    {
      {
        nix::DrvName dname( this->_fullName );
        this->_pname   = dname.name;
        this->_version = dname.version;
      }
      this->init( checkDrv );
    }


    FlakePackage(       Cursor                     cursor
                , const std::vector<nix::Symbol> & path
                ,       nix::SymbolTable         * symtab
                ,       bool                       checkDrv = true
                )
      : _cursor( cursor )
      , _fullName( cursor->getAttr( "name" )->getString() )
    {
      {
        nix::DrvName dname( this->_fullName );
        this->_pname   = dname.name;
        this->_version = dname.version;
      }
      for ( auto & p : symtab->resolve( cursor->getAttrPath() ) )
        {
          this->_pathS.push_back( p );
        }
      this->init( checkDrv );
    }


    FlakePackage( Cursor             cursor
                , nix::SymbolTable * symtab
                , bool               checkDrv = true
                )
      : FlakePackage( cursor, cursor->getAttrPath(), symtab, checkDrv )
    {}


/* -------------------------------------------------------------------------- */

    std::vector<std::string> getOutputsToInstallS() const;
    std::optional<bool>      isBroken()             const override;
    std::optional<bool>      isUnfree()             const override;


      std::vector<std::string_view>
    getOutputsToInstall() const override
    {
      std::vector<std::string_view> rsl;
      for ( auto & s : this->getOutputsToInstallS() ) { rsl.emplace_back( s ); }
      return rsl;
    }

      std::vector<std::string_view>
    getPathStrs() const override
    {
      std::vector<std::string_view> path;
      for ( const auto & p : this->_pathS ) { path.emplace_back( p ); }
      return path;
    }

    std::string_view getFullName() const override { return this->_fullName; }
    std::string_view getPname()    const override { return this->_pname;    }

    Cursor       getCursor()      const          { return this->_cursor;  }
    subtree_type getSubtreeType() const override { return this->_subtree; }

      nix::DrvName
    getParsedDrvName() const override
    {
      return nix::DrvName( this->_fullName );
    }

      std::optional<std::string_view>
    getVersion() const override
    {
      if ( this->_version.empty() ) { return std::nullopt;   }
      else                          { return this->_version; }
    }

      std::optional<std::string_view>
    getSemver() const override
    {
      return this->_semver;
    }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_subtree != ST_CATALOG ) { return std::nullopt; }
      return this->_pathS[2];
    }

      std::optional<std::string_view>
    getLicense() const override
    {
      if ( this->_license.has_value() ) { return this->_license; }
      else                              { return std::nullopt;   }
    }

      std::vector<std::string>
    getOutputsS() const
    {
      MaybeCursor o = this->_cursor->maybeGetAttr( "outputs" );
      if ( o == nullptr ) { return { "out" };             }
      else                { return o->getListOfStrings(); }
    }

      std::vector<std::string_view>
    getOutputs() const override
    {
      MaybeCursor o = this->_cursor->maybeGetAttr( "outputs" );
      if ( o == nullptr )
        {
          return { "out" };
        }
      else
        {
          std::vector<std::string_view> rsl;
          for ( const auto & p : o->getListOfStrings() )
            {
              rsl.emplace_back( p );
            }
          return rsl;
        }
      std::vector<std::string_view> rsl;
      for ( auto & s : this->getOutputsS() ) { rsl.emplace_back( s ); }
      return rsl;
    }

      std::optional<std::string>
    getDescription() const override
    {
      if ( ! this->_hasMetaAttr ) { return std::nullopt; }
      MaybeCursor l =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "description" );
      if ( l == nullptr ) { return std::nullopt; }
      try { return l->getString(); } catch( ... ) { return std::nullopt; }
    }


/* -------------------------------------------------------------------------- */

};  /* End class `FlakePackage' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
