/* ========================================================================== *
 *
 * A `PackageSet' implementation that leverages both `FlakePackageSet' and
 * `DbPackageSet' internally to "intelligently" select the optimal source.
 * In cases where a package definition is not available in a `DrvDb' it will be
 * evaluated and cached to optimize future lookups.
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <filesystem>
#include "flox/util.hh"
#include "flox/raw-package.hh"
#include "flox/flake-package-set.hh"
#include "flox/db-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class CachedPackageSet : public PackageSet {

  private:

    subtree_type                             _subtree;
    std::string                              _system;
    std::optional<std::string>               _stability;
    std::shared_ptr<nix::flake::LockedFlake> _flake;
    nix::ref<nix::EvalState>                 _state;
    std::shared_ptr<FlakePackageSet>         _fps;
    std::shared_ptr<DbPackageSet>            _dbps;
    std::shared_ptr<DrvDb>                   _db;
    bool                                     _populateDb = false;


/* -------------------------------------------------------------------------- */

      nix::ref<FlakePackageSet>
    getFlakePackageSet()
    {
      if ( this->_fps == nullptr )
        {
          this->_fps = std::make_shared<FlakePackageSet>(
            this->_state
          , this->_flake
          , this->_subtree
          , this->_system
          , this->_stability
          );
        }
      return (nix::ref<FlakePackageSet>) this->_fps;
    }

      nix::ref<DbPackageSet>
    getDbPackageSet()
    {
      if ( this->_dbps == nullptr )
        {
          this->_dbps = std::make_shared<DbPackageSet>(
            this->_flake
          , this->_subtree
          , this->_system
          , this->_stability
          );
        }
      return (nix::ref<DbPackageSet>) this->_dbps;
    }


/* -------------------------------------------------------------------------- */

  public:

    CachedPackageSet(
            nix::ref<nix::EvalState>                   state
    ,       std::shared_ptr<nix::flake::LockedFlake>   flake
    , const subtree_type                             & subtree
    ,       std::string_view                           system
    , const std::optional<std::string_view>          & stability = std::nullopt
    ) : _state( state )
      , _subtree( subtree )
      , _system( system )
      , _stability( stability )
      , _flake( flake )
    {
      /* Determine the Db status.
       * We may be creating from scratch, or filling a missing package set. */
      if ( ! std::filesystem::exists( getDrvDbName( * this->_flake ) ) )
        {
          this->_populateDb = true;
        }
      else
        {
          DrvDb db( this->_flake->getFingerprint(), false, false );
          progress_status s = db.getProgress(
            subtreeTypeToString( this->_subtree )
          , this->_system
          );

          if ( s == DBPS_INFO_DONE ) { this->_populateDb = false; }
          else                       { this->_populateDb = true;  }
        }

      if ( this->_populateDb )
        {
          this->_db = std::make_shared<DrvDb>( this->_flake->getFingerprint() );
          this->getFlakePackageSet();
        }
      else
        {
          this->getDbPackageSet();
        }
    }

    CachedPackageSet(
            nix::ref<nix::EvalState>          state
    , const FloxFlakeRef                    & flakeRef
    , const subtree_type                    & subtree
    ,       std::string_view                  system
    , const std::optional<std::string_view> & stability = std::nullopt
    ,       bool                              trace     = false
    ) : CachedPackageSet(
          state
        , std::make_shared<nix::flake::LockedFlake>(
            nix::flake::lockFlake( * state, flakeRef, floxFlakeLockFlags )
          )
        , subtree
        , system
        , stability
        )
    {}


/* -------------------------------------------------------------------------- */

      nix::flake::Fingerprint
    getFingerprint() const
    {
      return this->_flake->getFingerprint();
    }


/* -------------------------------------------------------------------------- */

    std::string_view getType()    const override { return "cached";       }
    subtree_type     getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability.value(); }
      else                                { return std::nullopt;             }
    }

      FloxFlakeRef
    getRef() const override {
      return this->_flake->flake.lockedRef;
    }


/* -------------------------------------------------------------------------- */

    bool        hasRelPath( const std::list<std::string_view> & path ) override;
    std::size_t size() override;

    std::shared_ptr<Package> maybeGetRelPath(
      const std::list<std::string_view> & path
    ) override;


/* -------------------------------------------------------------------------- */

    struct const_iterator
    {
      using value_type = const RawPackage;
      using reference  = value_type &;
      using pointer    = nix::ref<value_type>;

      private:
        std::shared_ptr<value_type>                      _ptr;
        std::shared_ptr<FlakePackageSet::const_iterator> _fi;
        std::shared_ptr<FlakePackageSet::const_iterator> _fe;
        std::shared_ptr<DbPackageSet::const_iterator>    _di;
        std::shared_ptr<DbPackageSet::const_iterator>    _de;
        std::shared_ptr<DrvDb>                           _db;
        bool                                             _populateDb;

        void loadPkg();

      public:
        const_iterator() = default;

        explicit const_iterator( bool                             populateDb
                               , std::shared_ptr<FlakePackageSet> fps
                               , std::shared_ptr<DbPackageSet>    dbps
                               , std::shared_ptr<DrvDb>           db
                               )
          : _populateDb( populateDb ), _db( db )
        {
          if ( _populateDb )
            {
              this->_fi = std::make_shared<FlakePackageSet::const_iterator>(
                fps->begin()
              );
              this->_fe = std::make_shared<FlakePackageSet::const_iterator>(
                fps->end()
              );
              assert( db != nullptr );
              if ( ( * this->_fi ) != ( * this->_fe ) ) { loadPkg(); }
            }
          else
            {
              this->_di = std::make_shared<DbPackageSet::const_iterator>(
                dbps->begin()
              );
              this->_de = std::make_shared<DbPackageSet::const_iterator>(
                dbps->end()
              );
              if ( ( * this->_di ) != ( * this->_de ) ) { loadPkg(); }
            }
        }


        std::string_view getType() const { return "cached"; }

        const_iterator & operator++();

          const_iterator
        operator++( int )
        {
          const_iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const const_iterator & other ) const
        {
          return this->_ptr == other._ptr;
        }

          bool
        operator!=( const const_iterator & other ) const
        {
          return this->_ptr != other._ptr;
        }

        reference operator*() { return * this->_ptr;                      }
        pointer  operator->() { return (nix::ref<value_type>) this->_ptr; }

    };  /* End struct `CachedPackageSet::const_iterator' */


/* -------------------------------------------------------------------------- */

    const_iterator begin() const;
    const_iterator end()   const { return const_iterator(); }


/* -------------------------------------------------------------------------- */

};  /* End class `CachedPackageSet' */


/* -------------------------------------------------------------------------- */

/**
 * Convert a `FlakePackageSet' to a `DbPackageSet' by writing its contents to
 * a database.
 */
DbPackageSet cachePackageSet( FlakePackageSet & ps );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
