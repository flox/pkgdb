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
Resolved::clear()
{
  this->input.name.clear();
  this->path.clear();
  this->input.locked = nlohmann::json::object();
  this->info         = nlohmann::json::object();
}


/* -------------------------------------------------------------------------- */

  std::vector<Resolved>
resolve_v0( ResolverState & state, const Descriptor & descriptor, bool one )
{
  std::vector<Resolved> rsl;
  for ( auto & [name, input] : * state.getPkgDbRegistry() )
    {
      if ( descriptor.input.has_value() && name != descriptor.input.value() )
        {
          continue;
        }
      auto args = state.getPkgQueryArgs( name );

      if ( descriptor.stability.has_value() )
        {
          /* If the input lacks this stability, skip. */
          if ( ( args.stabilities.has_value() ) &&
               ( std::find( args.stabilities->begin()
                          , args.stabilities->end()
                          , * descriptor.stability
                          ) == args.stabilities->end()
               )
             )
            {
              continue;
            }
          /* Otherwise, force catalog resolution. */
          args.subtrees = std::vector<Subtree> { ST_CATALOG };
        }

      /* If the input lacks the subtree we need then skip. */
      if ( args.subtrees.has_value() && ( descriptor.subtree.has_value() ) )
        {
          if ( std::find( args.subtrees->begin()
                        , args.subtrees->end()
                        , Subtree( * descriptor.subtree )
                        ) == args.subtrees->end()
             )
            {
              continue;
            }
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
          auto resolved = Resolved {
            .input = Resolved::Input {
              .name   = name
            , .locked = dbRO->lockedRef.attrs
            }
          , .path = dbRO->getPackagePath( row )
          , .info = dbRO->getPackage( row )
          };
          rsl.push_back( resolved );
          if ( one ) { return rsl; }
        }
    }

  return rsl;
}


/* -------------------------------------------------------------------------- */

} /* End namespace `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
