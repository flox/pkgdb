/* ========================================================================== *
 *
 * @file resolver/resolve.cc
 *
 * @brief Resolve package descriptors in flakes.
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/resolver/resolve.hh"
#include "flox/core/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
Resolved::Input::limitLocked()
{
  if ( this->locked.is_object() )
    {
      auto type = this->locked.at( "type" ).get<std::string>();
      if ( ! ( ( type == "path" ) || ( type == "tarball" )
               || ( type == "file" ) ) )
        {
          this->locked.erase( "narHash" );
        }
      this->locked.erase( "lastModified" );
      this->locked.erase( "lastModifiedDate" );
      this->locked.erase( "revCount" );
      this->locked.erase( "shortRev" );
    }
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
resolve_v0( ResolverState &state, const Descriptor &descriptor, bool one )
{
  std::vector<Resolved> rsl;
  for ( auto &[name, input] : *state.getPkgDbRegistry() )
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
      for ( const auto &row : rows )
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

void
from_json( const nlohmann::json &jfrom, Resolved &resolved )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw ParseResolvedException(
        "resolved installable must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }
  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "input" )
        {
          /* Rely on the underlying exception handlers. */
          value.get_to( resolved.input );
        }
      else if ( key == "path" )
        {
          try
            {
              value.get_to( resolved.path );
            }
          catch ( nlohmann::json::exception &e )
            {
              throw ParseResolvedException(
                "couldn't interpret field 'path'",
                flox::extract_json_errmsg( e ).c_str() );
            }
        }
      else if ( key == "info" ) { resolved.info = value; }
      else
        {
          throw ParseResolvedException(
            "encountered unrecognized field '" + key
            + "' while parsing resolved installable" );
        }
    }
}

void
to_json( nlohmann::json &jto, const Resolved &resolved )
{
  jto = { { "input", resolved.input },
          { "path", resolved.path },
          { "info", resolved.info } };
}


void
from_json( const nlohmann::json &jfrom, Resolved::Input &input )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw ParseResolvedException( "registry input must be an object, but is"
                                    + aOrAn + std::string( jfrom.type_name() )
                                    + '.' );
    }
  for ( const auto &[key, value] : jfrom.items() )
    {
      if ( key == "name" )
        {
          try
            {
              value.get_to( input.name );
            }
          catch ( nlohmann::json::exception &e )
            {
              throw ParseResolvedException(
                "couldn't parse resolved input field '" + key + "'",
                extract_json_errmsg( e ).c_str() );
            }
        }
      else if ( key == "locked" ) { input.locked = value; }
      else
        {
          throw ParseResolvedException( "encountered unrecognized field '" + key
                                        + "' while parsing locked input" );
        }
    }
}

void
to_json( nlohmann::json &jto, const Resolved::Input &input )
{
  jto = { { "name", input.name }, { "locked", input.locked } };
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
