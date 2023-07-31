/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
#include "flox/cached-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  bool
CachedPackageSet::hasRelPath( const std::list<std::string_view> & path )
{
  if ( this->_populateDb ) { return this->_fps->hasRelPath( path );  }
  else                     { return this->_dbps->hasRelPath( path ); }
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<Package>
CachedPackageSet::maybeGetRelPath( const std::list<std::string_view> & path )
{
  if ( this->_populateDb ) { return this->_fps->maybeGetRelPath( path );  }
  else                     { return this->_dbps->maybeGetRelPath( path ); }
}


/* -------------------------------------------------------------------------- */

  std::size_t
CachedPackageSet::size()
{
  if ( this->_populateDb ) { return this->_fps->size();  }
  else                     { return this->_dbps->size(); }
}


/* -------------------------------------------------------------------------- */

  CachedPackageSet::const_iterator
CachedPackageSet::begin() const
{
  if ( this->_populateDb )
    {
      return const_iterator(
        this->_populateDb
      , this->_fps
      , nullptr
      , this->_db
      );
    }
  else
    {
      return const_iterator( this->_populateDb, nullptr, this->_dbps, nullptr );
    }
}


/* -------------------------------------------------------------------------- */

  void
CachedPackageSet::const_iterator::loadPkg()
{
  /* If we are populating the DB then evaluate the next package, cache the
   * result in our DB, then return the result. */
  if ( this->_populateDb )
    {
      const Package * p = this->_fi->operator->().get_ptr().get();

      this->_db->setDrvInfo( * p );

      std::vector<std::string_view> pathS;
      for ( const auto & s : p->getPathStrs() ) { pathS.push_back( s ); }

      std::vector<std::string_view> outputs;
      for ( const auto & s : p->getOutputs() ) { outputs.push_back( s ); }

      std::vector<std::string_view> outputsToInstall;
      for ( const auto & s : p->getOutputsToInstall() )
        {
          outputsToInstall.push_back( s );
        }

      this->_ptr = std::make_shared<value_type>(
        pathS
      , p->getFullName()
      , p->getPname()
      , p->getVersion()
      , p->getSemver()
      , p->getLicense()
      , outputs
      , outputsToInstall
      , p->isBroken()
      , p->isUnfree()
      , p->hasMetaAttr()
      , p->hasPnameAttr()
      , p->hasVersionAttr()
      );

    }
  else
    {
      this->_ptr = std::static_pointer_cast<value_type>(
        this->_di->operator->().get_ptr()
      );
    }
}


/* -------------------------------------------------------------------------- */

  CachedPackageSet::const_iterator &
CachedPackageSet::const_iterator::operator++()
{
  /* If we are populating the DB then evaluate the next package, cache the
   * result in our DB, then return the result. */
  if ( this->_populateDb )
    {
      ++( * this->_fi );
      if ( ( * this->_fi ) != ( * this->_fe ) ) { loadPkg(); }
    }
  else
    {
      ++( * this->_di );
      if ( ( * this->_di ) != ( * this->_de ) ) { loadPkg(); }
    }

  return * this;
}  /* End `CachedPackageSet::const_iterator::operator++()' */


/* -------------------------------------------------------------------------- */

  DbPackageSet
cachePackageSet( FlakePackageSet & ps )
{
  std::shared_ptr<DrvDb> db = std::make_shared<DrvDb>( ps.getFingerprint() );
  /* Check to see if this db already exists and is "done". */
  progress_status s = db->getProgress( subtreeTypeToString( ps.getSubtree() )
                                     , ps.getSystem()
                                     );

  if ( ! ( ( s == DBPS_INFO_DONE ) ||
           /* Check if stability is done by comparing sizes. */
           ( ( ps.getSubtree() == ST_CATALOG ) &&
             ( s == DBPS_PARTIAL ) &&
             ( db->countDrvInfosStability( ps.getSystem()
                                         , ps.getStability().value()
                                         )
               == ps.size()
             )
           )
         )
     )
    {
      /* Populate the DB. */
      for ( const FlakePackage & pkg : ps )
        {
          db->setDrvInfo( (const Package &) pkg );
        }

      /* Mark subtree/system as "done". */
      // TODO: find a way to mark catalog stabilities.
      if ( ps.getSubtree() != ST_CATALOG )
        {
          db->promoteProgress( subtreeTypeToString( ps.getSubtree() )
                             , ps.getSystem()
                             , DBPS_INFO_DONE
                             );
        }
      else
        {
          db->promoteProgress( subtreeTypeToString( ps.getSubtree() )
                             , ps.getSystem()
                             , DBPS_PARTIAL
                             );
        }
    }

   return DbPackageSet( ps.getFlake()
                      , db
                      , ps.getSubtree()
                      , ps.getSystem()
                      , ps.getStability()
                      );
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
