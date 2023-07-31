/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/names.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class FlakePackage : public Package {

  private:
    Cursor                   _cursor;
    std::vector<nix::Symbol> _path;
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

    void init( bool checkDrv = true );


/* -------------------------------------------------------------------------- */

  public:

    FlakePackage(       Cursor                     cursor
                , const std::vector<nix::Symbol> & path
                ,       nix::SymbolTable         * symtab
                ,       bool                       checkDrv = true
                )
      : _cursor( cursor )
      , _path( path )
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

    std::vector<std::string> getOutputsToInstall() const override;
    std::optional<bool>      isBroken()            const override;
    std::optional<bool>      isUnfree()            const override;

    std::vector<nix::Symbol> getPath() const { return this->_path; }

      std::vector<std::string>
    getPathStrs() const override
    {
      return this->_pathS;
    }

    std::string  getPname()       const override { return this->_pname;    }
    Cursor       getCursor()      const          { return this->_cursor;   }
    subtree_type getSubtreeType() const override { return this->_subtree;  }
    std::string  getFullName()    const override { return this->_fullName; }

    bool hasMetaAttr()    const override { return this->_hasMetaAttr;    }
    bool hasPnameAttr()   const override { return this->_hasPnameAttr;   }
    bool hasVersionAttr() const override { return this->_hasVersionAttr; }

      nix::DrvName
    getParsedDrvName() const override
    {
      return nix::DrvName( this->_fullName );
    }

      std::optional<std::string>
    getVersion() const override
    {
      if ( this->_version.empty() ) { return std::nullopt;   }
      else                          { return this->_version; }
    }

      std::optional<std::string>
    getSemver() const override
    {
      return this->_semver;
    }

      std::optional<std::string>
    getStability() const override
    {
      if ( this->_subtree != ST_CATALOG ) { return std::nullopt; }
      return this->_pathS[2];
    }

      std::optional<std::string>
    getLicense() const override
    {
      if ( ! this->_hasMetaAttr ) { return std::nullopt; }
      MaybeCursor l =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "license" );
      if ( l == nullptr ) { return std::nullopt; }
      try { return l->getAttr( "spdxId" )->getString(); }
      catch( ... ) { return std::nullopt; }
    }

      std::vector<std::string>
    getOutputs() const override
    {
      MaybeCursor o = this->_cursor->maybeGetAttr( "outputs" );
      if ( o == nullptr ) { return { "out" }; }
      else { return o->getListOfStrings(); }
    }


/* -------------------------------------------------------------------------- */

};  /* End class `FlakePackage' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
