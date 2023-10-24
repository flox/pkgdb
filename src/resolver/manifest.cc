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

std::optional<InvalidManifestFileException>
ManifestRaw::EnvBase::check() const
{
  if ( this->floxhub.has_value() && this->dir.has_value() )
    {
      return InvalidManifestFileException(
        "Manifest may only define one of `env-base.floxhub' or `env-base.dir' "
        "fields." );
    }
  return std::nullopt;
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
}

// TODO: remove `maybe_unused' when you write `to_json' for `ManifestRaw'.
[[maybe_unused]] static void
to_json( nlohmann::json & jto, const ManifestRaw::EnvBase & env )
{
  if ( auto err = env.check(); err.has_value() ) { throw *err; }
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
                "Failed to parse manifest field `env-base.options.semver."
                "prefer-pre-releases' with value: " + value.dump() );
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


[[maybe_unused]] static void
to_json( nlohmann::json & jto, const ManifestRaw::Options::Semver semver )
{
  if ( semver.preferPreReleases.has_value() )
    {
      jto = { { "prefer-pre-releases", *semver.preferPreReleases } };
    }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

// TODO: Write explicit definitions with exception handling.

/* Generate `to_json' and `from_json' `ManifestRaw::Options' */

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT( ManifestRaw::Options::Allows,
                                                 unfree,
                                                 broken,
                                                 licenses )

// TODO: Remap `fooBar' to `foo-bar' in the JSON.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT( ManifestRaw::Options,
                                                 systems,
                                                 allow,
                                                 semver,
                                                 packageGroupingStrategy,
                                                 activationStrategy )


/* Generate `to_json' and `from_json' `ManifestRaw::Hook' */
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT( ManifestRaw::Hook,
                                                 script,
                                                 file )


/* -------------------------------------------------------------------------- */

std::optional<InvalidManifestFileException>
ManifestRaw::Hook::check() const
{
  if ( this->script.has_value() && this->file.has_value() )
    {
      return InvalidManifestFileException(
        "Hook may only define one of `hook.script' or `hook.file' fields." );
    }
  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

std::optional<std::string>
ManifestRaw::Hook::getHook() const
{
  if ( auto err = this->check(); err.has_value() ) { throw *err; }
  if ( this->file.has_value() )
    {
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
  else { return this->script; }
}


/* -------------------------------------------------------------------------- */

/**
 * @brief Add a `name' field to the descriptor if it is missing, and no
 *        other accebtable field is present.
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

void
from_json( const nlohmann::json & jfrom, ManifestRaw & manifest )
{
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
          // TODO: iterate for improved exception messages.
          try
            {
              value.get_to( manifest.vars );
            }
          catch ( const nlohmann::json::exception & err )
            {
              throw InvalidManifestFileException(
                "Invalid value for `vars' field: "
                + std::string( err.what() ) );
            }
        }
      else if ( key == "hook" )
        {
          try
            {
              value.get_to( manifest.hook );
            }
          catch ( const nlohmann::json::exception & err )
            {
              // TODO: better exception message.
              throw InvalidManifestFileException(
                "Invalid value for `hook' field: "
                + std::string( err.what() ) );
            }
          if ( auto err = manifest.hook->check(); err.has_value() )
            {
              throw *err;
            }
        }
      else if ( key == "options" )
        {
          // TODO: Write good exception messages in `Options' `from_json'.
          try
            {
              value.get_to( manifest.options );
            }
          catch ( const nlohmann::json::exception & err )
            {
              throw InvalidManifestFileException(
                "Invalid value for `options' field: "
                + std::string( err.what() ) );
            }
        }
      else if ( key == "env-base" )
        {
          try
            {
              value.get_to( manifest.envBase );
            }
          catch ( const nlohmann::json::exception & err )
            {
              throw InvalidManifestFileException(
                "Invalid value for `env-base' field: "
                + std::string( err.what() ) );
            }
          if ( auto err = manifest.envBase->check(); err.has_value() )
            {
              throw *err;
            }
        }
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
