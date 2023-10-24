/* ========================================================================== *
 *
 * @file resolver/manifest.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>

#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

void
ManifestRaw::EnvBase::check() const
{
  if ( this->floxhub.has_value() && this->dir.has_value() )
    {
      throw InvalidManifestFileException(
        "Manifest may only define one of `env-base.floxhub' or `env-base.dir' "
        "fields." );
    }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::EnvBase & env )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest field `options.env-base' must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  /* Clear fields. */
  env.dir     = std::nullopt;
  env.floxhub = std::nullopt;

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "floxhub" )
        {
          try
            {
              value.get_to( env.floxhub );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `env-base.floxhub' with value: "
                + value.dump() );
            }
        }
      else if ( key == "dir" )
        {
          try
            {
              value.get_to( env.dir );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `env-base.dir' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "Unrecognized manifest field `env-base." + key + "'." );
        }
    }
  env.check();
}

// TODO: remove `maybe_unused' when you write `to_json' for `ManifestRaw'.
[[maybe_unused]] static void
to_json( nlohmann::json & jto, const ManifestRaw::EnvBase & env )
{
  env.check();
  if ( env.dir.has_value() ) { jto = { { "dir", *env.dir } }; }
  else if ( env.floxhub.has_value() ) { jto = { { "floxhub", *env.floxhub } }; }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::Options::Semver & semver )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest field `options.semver' must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  /* Clear fields. */
  semver.preferPreReleases = std::nullopt;

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "prefer-pre-releases" )
        {
          try
            {
              value.get_to( semver.preferPreReleases );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `options.semver."
                "prefer-pre-releases' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "Unrecognized manifest field `env-base.options.semver." + key
            + "'." );
        }
    }
}


static void
to_json( nlohmann::json & jto, const ManifestRaw::Options::Semver & semver )
{
  if ( semver.preferPreReleases.has_value() )
    {
      jto = { { "prefer-pre-releases", *semver.preferPreReleases } };
    }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::Options::Allows & allow )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest field `options.allow' must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  /* Clear fields. */
  allow.licenses = std::nullopt;
  allow.unfree   = std::nullopt;
  allow.broken   = std::nullopt;

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "unfree" )
        {
          try
            {
              value.get_to( allow.unfree );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `options.allow.unfree' "
                "with value: "
                + value.dump() );
            }
        }
      else if ( key == "broken" )
        {
          try
            {
              value.get_to( allow.broken );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `options.allow.broken' "
                "with value: "
                + value.dump() );
            }
        }
      else if ( key == "licenses" )
        {
          try
            {
              value.get_to( allow.licenses );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `options.allow.licenses' "
                "with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "Unrecognized manifest field `options.allow." + key + "'." );
        }
    }
}


static void
to_json( nlohmann::json & jto, const ManifestRaw::Options::Allows & allow )
{
  if ( allow.unfree.has_value() ) { jto = { { "unfree", *allow.unfree } }; }
  else { jto = nlohmann::json::object(); }

  if ( allow.broken.has_value() ) { jto.emplace( "broken", *allow.broken ); }
  if ( allow.licenses.has_value() )
    {
      jto.emplace( "licenses", *allow.licenses );
    }
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json & jfrom, ManifestRaw::Options & opts )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest field `options' must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "systems" )
        {
          try
            {
              value.get_to( opts.systems );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `options.systems' with value: "
                + value.dump() );
            }
        }
      else if ( key == "allow" )
        {
          /* Rely on the underlying exception handlers. */
          value.get_to( opts.allow );
        }
      else if ( key == "semver" )
        {
          /* Rely on the underlying exception handlers. */
          ManifestRaw::Options::Semver semver;
          value.get_to( semver );
          opts.semver = semver;
        }
      else if ( key == "package-grouping-strategy" )
        {
          try
            {
              value.get_to( opts.packageGroupingStrategy );
            }
          catch ( const nlohmann::json & exception )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field "
                "`options.package-grouping-strategy' with value: "
                + value.dump() );
            }
        }
      else if ( key == "activation-strategy" )
        {
          try
            {
              value.get_to( opts.activationStrategy );
            }
          catch ( const nlohmann::json & exception )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field "
                "`options.activation-strategy' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "Unrecognized manifest field `options." + key + "'." );
        }
    }
}


[[maybe_unused]] void
to_json( nlohmann::json & jto, const ManifestRaw::Options & opts )
{
  if ( opts.systems.has_value() ) { jto = { { "systems", *opts.systems } }; }
  else { jto = nlohmann::json::object(); }

  if ( opts.allow.has_value() ) { jto.emplace( "allow", *opts.allow ); }

  if ( opts.semver.has_value() ) { jto.emplace( "semver", *opts.semver ); }

  if ( opts.packageGroupingStrategy.has_value() )
    {
      jto.emplace( "package-grouping-strategy", *opts.packageGroupingStrategy );
    }

  if ( opts.activationStrategy.has_value() )
    {
      jto.emplace( "activation-strategy", *opts.activationStrategy );
    }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::Hook & hook )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest field `options' must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  /* Clear fields. */
  hook.file   = std::nullopt;
  hook.script = std::nullopt;

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "script" )
        {
          try
            {
              value.get_to( hook.script );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `hook.script' with value: "
                + value.dump() );
            }
        }
      else if ( key == "file" )
        {
          try
            {
              value.get_to( hook.file );
            }
          catch ( const nlohmann::json::exception & )
            {
              throw InvalidManifestFileException(
                "Failed to parse manifest field `hook.file' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "Unrecognized manifest field `hook." + key + "'." );
        }
    }

  hook.check();
}


[[maybe_unused]] static void
to_json( nlohmann::json & jto, const ManifestRaw::Hook & hook )
{
  hook.check();
  if ( hook.file.has_value() ) { jto = { { "file", *hook.file } }; }
  else if ( hook.script.has_value() ) { jto = { { "script", *hook.script } }; }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

void
ManifestRaw::Hook::check() const
{
  if ( this->script.has_value() && this->file.has_value() )
    {
      throw InvalidManifestFileException(
        "Hook may only define one of `hook.script' or `hook.file' fields." );
    }
}


/* -------------------------------------------------------------------------- */

std::optional<std::string>
ManifestRaw::Hook::getHook() const
{
  this->check();
  if ( this->file.has_value() )
    {
      // TODO: This assumes that PWD is the same directory as the manifest.
      // You need to track the `manifestPath' more explicitly to resolve any
      // relative paths that may appear here.
      std::filesystem::path path( *this->file );
      if ( ! std::filesystem::exists( path ) )
        {
          throw InvalidManifestFileException( "Hook file `" + path.string()
                                              + "' does not exist." );
        }
      std::ifstream      file( path );
      std::ostringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }
  return this->script;
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Add a `name' field to the descriptor if it is missing, and no
 *        other acceptable field is present.
 *
 * The fields `absPath` or `path` may be used instead of the `name` field, but
 * if none are present use the `install.<ID>` identifier as `name`.
 *
 * @param[in] iid The `install.<ID>` identifier associated with the descriptor.
 * @param[in,out] desc The descriptor to fixup.
 */
static void
fixupDescriptor( const std::string & iid, ManifestDescriptor & desc )
{
  if ( ! ( desc.name.has_value() || desc.path.has_value() ) )
    {
      desc.name = iid;
    }
}


/* -------------------------------------------------------------------------- */

static void
varsFromJSON( const nlohmann::json &                         jfrom,
              std::unordered_map<std::string, std::string> & vars )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest `vars' field must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }
  vars.clear();
  for ( const auto & [key, value] : jfrom.items() )
    {
      std::string val;
      try
        {
          value.get_to( val );
        }
      catch ( const nlohmann::json::exception & err )
        {
          throw InvalidManifestFileException( "Invalid value for `vars." + key
                                              + "' with value: "
                                              + value.dump() );
        }
      vars.emplace( key, std::move( val ) );
    }
}


/* -------------------------------------------------------------------------- */

void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest )
{
  if ( ! jfrom.is_object() )
    {
      std::string aOrAn = jfrom.is_array() ? " an " : " a ";
      throw InvalidManifestFileException(
        "Manifest must be an object, but is" + aOrAn
        + std::string( jfrom.type_name() ) + '.' );
    }

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "install" )
        {
          for ( const auto & [name, desc] : value.items() )
            {
              /* An empty/null descriptor uses `name' of the attribute. */
              if ( desc.is_null() )
                {
                  ManifestDescriptor manDesc;
                  manDesc.name = name;
                  manifest.install.emplace( name, std::move( manDesc ) );
                }
              else  // TODO: strings
                {
                  auto manDesc
                    = ManifestDescriptor( desc.get<ManifestDescriptorRaw>() );
                  fixupDescriptor( name, manDesc );
                  manifest.install.emplace( name, std::move( manDesc ) );
                }
            }
        }
      else if ( key == "registry" ) { value.get_to( manifest.registry ); }
      else if ( key == "vars" )
        {
          std::unordered_map<std::string, std::string> vars;
          varsFromJSON( value, vars );
          manifest.vars = std::move( vars );
        }
      else if ( key == "hook" ) { value.get_to( manifest.hook ); }
      else if ( key == "options" ) { value.get_to( manifest.options ); }
      else if ( key == "env-base" ) { value.get_to( manifest.envBase ); }
      else
        {
          throw InvalidManifestFileException( "Unrecognized manifest field: `"
                                              + key + "'." );
        }
    }
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
