/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
#include "flox/flake-package.hh"
#include "flox/flake-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  bool
FlakePackageSet::hasRelPath( const std::list<std::string_view> & path )
{
  MaybeCursor curr = this->openCursor();
  if ( curr == nullptr ) { return false; }
  try
    {
      for ( auto & p : path )
        {
          curr = curr->maybeGetAttr( p );
          if ( curr == nullptr ) { return false; }
        }
      /* NOTE: this doesn't guarantee eval will succeed when we try to get
       * fields like `name'.
       * This is the same category of issue we see in `size()' and iterator. */
      return curr->isDerivation();
    }
  catch( ... )
    {
      //nix::ignoreException();
      return false;
    }
  return true;
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<Package>
FlakePackageSet::maybeGetRelPath( const std::list<std::string_view> & path )
{
  MaybeCursor curr = this->openCursor();
  if ( curr == nullptr ) { return nullptr; }
  try
    {
      for ( auto & p : path )
        {
          curr = curr->maybeGetAttr( p );
          if ( curr == nullptr ) { return nullptr; }
        }
      return std::make_shared<FlakePackage>(
        (Cursor) curr
      , & this->_state->symbols
      , false
      );
    }
  catch( ... )
    {
      //nix::ignoreException();
      return nullptr;
    }
}


/* -------------------------------------------------------------------------- */

  std::size_t
FlakePackageSet::size()
{
  MaybeCursor curr = this->openCursor();
  if ( curr == nullptr ) { return 0; }

  /* We intentionally avoid a `try' block for `packages'. */
  if ( this->_subtree == ST_PACKAGES ) { return curr->getAttrs().size(); }

  std::size_t rsl = 0;
  todo_queue todos;
  todos.push( std::move( (Cursor) curr ) );
  while ( ! todos.empty() )
    {
      for ( const nix::Symbol & s : todos.front()->getAttrs() )
        {
          try
            {
              MaybeCursor c = todos.front()->getAttr( s );
              if ( c == nullptr ) { continue; }
              if ( c->isDerivation() )
                {
                  if ( ! c->getAttr( "name" )->getString().empty() ) { ++rsl; }
                }
              else
                {
                  MaybeCursor m = c->maybeGetAttr( "recurseForDerivations" );
                  if ( ( m != nullptr ) && m->getBool() )
                    {
                      todos.push( (Cursor) c );
                    }
                }
            }
          catch( ... )
            {
              /* If eval fails ignore the package. */
              //nix::ignoreException();
            }
        }
      todos.pop();
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  FlakePackageSet::const_iterator
FlakePackageSet::begin()
{
  MaybeCursor curr = this->openCursor();
  todo_queue todo;
  todo.emplace( std::move( curr ) );
  return const_iterator( this->_subtree
                       , & this->_state->symbols
                       , std::move( todo )
                       );
}


/* -------------------------------------------------------------------------- */

  bool
FlakePackageSet::const_iterator::evalPackage()
{
  if ( ( this->_todo.empty() ) || ( this->_syms.empty() ) )
    {
      this->_ptr = nullptr;
      return false;
    }

  Cursor c = this->_todo.front();
  try
    {
      MaybeCursor m = c->maybeGetAttr( this->_syms.front() );
      if ( m == nullptr )
        {
          this->_ptr = nullptr;
          return false;
        }
      else
        {
          c = (Cursor) m;
        }

      if ( ( this->_subtree == ST_PACKAGES ) || c->isDerivation() )
        {
          this->_ptr = std::make_shared<FlakePackage>(
            c
          , this->_symtab
          , false          /* checkDrv */
          );
          return true;
        }
    }
  catch( ... )
    {
      /* If eval fails ignore the package. */
      // nix::ignoreException();
    }
  this->_ptr = nullptr;
  return false;
}



/* -------------------------------------------------------------------------- */


  FlakePackageSet::const_iterator &
FlakePackageSet::const_iterator::operator++()
{
  /* We are "seeking" for packages using a queue of child iterator that loops
   * over attributes.
   * If we've reached the end of our search, mark a phony sentinel value.
   * That is "we fill phony values" that are recognized as a marker. */
    if ( this->_todo.empty() ) { return this->clear(); }

    /* Go to the next attribute in our current iterator. */
    if ( ! this->_syms.empty() ) { this->_syms.pop(); }

    /* If we hit the end either start processing the next `todo' list member,
     * or bail if it's empty. */
    if ( this->_syms.empty() )
      {
        //std::cerr << "Reached end of: " << this->_todo.front()->getAttrPathStr()
        //          << std::endl;
        this->_todo.pop();
        if ( this->_todo.empty() )
          {
            /* Set to sentinel value and bail. */
            return this->clear();
          }
        else
          {
            /* Start processing the next cursor by filling the symbol queue. */
            for ( auto & s : this->_todo.front()->getAttrs() )
              {
                this->_syms.push( s );
              }
            /* In the unlikely event that we get an empty attrset,
             * keep seeking. */
            if ( this->_syms.empty() ) { return ++( * this ); }
          }
      }

    /* If the cursor is at a package, we're done. */
    if ( evalPackage() ) { assert( this->_ptr != nullptr ); return * this; }

    /* See if we have a package, or if we might need to recurse into a
     * sub-attribute for more packages ( this only occurs for some subtrees ) */
    try
      {
        MaybeCursor c =
          this->_todo.front()->maybeGetAttr( this->_syms.front() );
        if ( c != nullptr )
          {
            MaybeCursor m = c->maybeGetAttr( "recurseForDerivations" );
            if ( ( m != nullptr ) && m->getBool() )
              {
                //std::cerr << "Pushing: " << c->getAttrPathStr() << std::endl;
                this->_todo.push( std::move( (Cursor) c ) );
              }
          }
      }
    catch( ... )
      {
        // NOTE: this causes messages to be printed to `stderr'.
        // nix::ignoreException();
      }
    /* We didn't hit a package, we need to keep searching. */
    return ++( * this );
}  /* End `FlakePackageSet::const_iterator::operator++()' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
