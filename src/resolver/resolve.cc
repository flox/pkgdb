/* ========================================================================== *
 *
 * @file resolver/resolve.cc
 *
 * @brief Resolve package descriptors in flakes.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
Resolved::Input::limitLocked()
{
  auto type = this->locked.at( "type" ).get<std::string>();
  if ( ! ( ( type == "path" ) || ( type == "tarball" ) || ( type == "file" ) ) )
    {
      this->locked.erase( "narHash" );
    }
  this->locked.erase( "lastModified" );
  this->locked.erase( "lastModifiedDate" );
  this->locked.erase( "revCount" );
  this->locked.erase( "shortRev" );
}


/* -------------------------------------------------------------------------- */

void
Resolved::clear()
{
  this->input.name   = std::nullopt;
  this->input.locked = nullptr;
  this->path.clear();
  this->info = nullptr;
}


/* -------------------------------------------------------------------------- */

std::vector<Resolved>
resolve_v0( ResolverState & state, const Descriptor & descriptor, bool one )
{
  std::vector<Resolved> rsl;
  for ( auto & [name, input] : *state.getPkgDbRegistry() )
    {
      if ( descriptor.input.has_value() && ( name != ( *descriptor.input ) ) )
        {
          continue;
        }
      auto args = state.getPkgQueryArgs( name );

      /* If the input lacks the subtree we need then skip. */
      if ( args.subtrees.has_value() && ( descriptor.subtree.has_value() )
           && ( std::find( args.subtrees->begin(),
                           args.subtrees->end(),
                           Subtree( *descriptor.subtree ) )
                == args.subtrees->end() ) )
        {
          continue;
        }

      /* Fill remaining args from descriptor. */
      descriptor.fillPkgQueryArgs( args );

      auto query = flox::pkgdb::PkgQuery( args );
      auto dbRO  = input->getDbReadOnly();
      auto rows  = query.execute( dbRO->db );

      /* A swing and a miss. */
      if ( rows.empty() ) { continue; }

      /* Fill `rsl' with resolutions. */
      for ( const auto & row : rows )
        {
          /* Strip some fields from the locked _flake ref_. */
          auto locked  = dbRO->lockedRef.attrs;
          auto info    = dbRO->getPackage( row );
          auto absPath = std::move( info.at( "absPath" ) );
          info.erase( "absPath" );

          rsl.emplace_back(
            Resolved { .input = Resolved::Input( name, std::move( locked ) ),
                       .path  = std::move( absPath ),
                       .info  = std::move( info ) } );
          if ( one ) { return rsl; }
        }
    }

  return rsl;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
