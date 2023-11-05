/* ========================================================================== *
 *
 * @file resolver/manifest-raw.cc
 *
 * @brief An abstract description of an environment in its unresolved state.
 *        This file contains the implementation of the
 *        @a flox::resolver::ManifestRaw struct, and associated JSON parsers.
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>

#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json &               jfrom,
           GlobalManifestRaw::Options::Semver & semver )
{
  assertIsJSONObject<InvalidManifestFileException>(
    jfrom,
    "manifest field `options.semver'" );

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
                "failed to parse manifest field `options.semver."
                "prefer-pre-releases' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized manifest field `env-base.options.semver." + key
            + "'." );
        }
    }
}


static void
to_json( nlohmann::json &                           jto,
         const GlobalManifestRaw::Options::Semver & semver )
{
  if ( semver.preferPreReleases.has_value() )
    {
      jto = { { "prefer-pre-releases", *semver.preferPreReleases } };
    }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json &               jfrom,
           GlobalManifestRaw::Options::Allows & allow )
{
  assertIsJSONObject<InvalidManifestFileException>(
    jfrom,
    "manifest field `options.allow'" );

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
                "failed to parse manifest field `options.allow.unfree' "
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
                "failed to parse manifest field `options.allow.broken' "
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
                "failed to parse manifest field `options.allow.licenses' "
                "with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized manifest field `options.allow." + key + "'." );
        }
    }
}


static void
to_json( nlohmann::json &                           jto,
         const GlobalManifestRaw::Options::Allows & allow )
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
from_json( const nlohmann::json & jfrom, GlobalManifestRaw::Options & opts )
{
  assertIsJSONObject<InvalidManifestFileException>(
    jfrom,
    "manifest field `options'" );

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
                "failed to parse manifest field `options.systems' with value: "
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
                "failed to parse manifest field "
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
                "failed to parse manifest field "
                "`options.activation-strategy' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized manifest field `options." + key + "'." );
        }
    }
}


void
to_json( nlohmann::json & jto, const GlobalManifestRaw::Options & opts )
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

void
from_json( const nlohmann::json & jfrom, GlobalManifestRaw & manifest )
{
  assertIsJSONObject<InvalidManifestFileException>( jfrom, "global manifest" );

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "registry" ) { value.get_to( manifest.registry ); }
      else if ( key == "options" ) { value.get_to( manifest.options ); }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized global manifest field: `" + key + "'." );
        }
    }
  manifest.check();
}


void
to_json( nlohmann::json & jto, const GlobalManifestRaw & manifest )
{
  manifest.check();
  jto = nlohmann::json::object();

  if ( manifest.options.has_value() ) { jto["options"] = *manifest.options; }
  if ( manifest.registry.has_value() ) { jto["registry"] = *manifest.registry; }
}


/* -------------------------------------------------------------------------- */

void
ManifestRaw::EnvBase::check() const
{
  if ( this->floxhub.has_value() && this->dir.has_value() )
    {
      throw InvalidManifestFileException(
        "manifest may only define one of `env-base.floxhub' or `env-base.dir' "
        "fields." );
    }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::EnvBase & env )
{
  assertIsJSONObject<InvalidManifestFileException>(
    jfrom,
    "manifest field `options.env-base'" );

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
                "failed to parse manifest field `env-base.floxhub' with value: "
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
                "failed to parse manifest field `env-base.dir' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized manifest field `env-base." + key + "'." );
        }
    }
  env.check();
}


static void
to_json( nlohmann::json & jto, const ManifestRaw::EnvBase & env )
{
  env.check();
  if ( env.dir.has_value() ) { jto = { { "dir", *env.dir } }; }
  else if ( env.floxhub.has_value() ) { jto = { { "floxhub", *env.floxhub } }; }
  else { jto = nlohmann::json::object(); }
}


/* -------------------------------------------------------------------------- */

static void
from_json( const nlohmann::json & jfrom, ManifestRaw::Hook & hook )
{
  assertIsJSONObject<InvalidManifestFileException>( jfrom,
                                                    "manifest field `hook'" );

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
                "failed to parse manifest field `hook.script' with value: "
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
                "failed to parse manifest field `hook.file' with value: "
                + value.dump() );
            }
        }
      else
        {
          throw InvalidManifestFileException(
            "unrecognized manifest field `hook." + key + "'." );
        }
    }

  hook.check();
}


static void
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
        "hook may only define one of `hook.script' or `hook.file' fields." );
    }
}


/* -------------------------------------------------------------------------- */

static void
varsFromJSON( const nlohmann::json &                         jfrom,
              std::unordered_map<std::string, std::string> & vars )
{
  assertIsJSONObject<InvalidManifestFileException>( jfrom,
                                                    "manifest field `vars'" );
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
          throw InvalidManifestFileException( "invalid value for `vars." + key
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
  assertIsJSONObject<InvalidManifestFileException>( jfrom, "manifest" );

  for ( const auto & [key, value] : jfrom.items() )
    {
      if ( key == "install" )
        {
          std::unordered_map<std::string, std::optional<ManifestDescriptorRaw>>
            install;
          for ( const auto & [name, desc] : value.items() )
            {
              /* An empty/null descriptor uses `name' of the attribute. */
              if ( desc.is_null() ) { install.emplace( name, std::nullopt ); }
              else  // TODO: parse CLI strings
                {
                  try
                    {
                      install.emplace( name,
                                       desc.get<ManifestDescriptorRaw>() );
                    }
                  catch ( const nlohmann::json::exception & )
                    {
                      throw InvalidManifestFileException(
                        "failed to parse manifest field `install." + name
                        + "'." );
                    }
                }
            }
          manifest.install = std::move( install );
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
          throw InvalidManifestFileException( "unrecognized manifest field: `"
                                              + key + "'." );
        }
    }
  manifest.check();
}


void
to_json( nlohmann::json & jto, const ManifestRaw & manifest )
{
  manifest.check();
  jto = nlohmann::json::object();

  if ( manifest.envBase.has_value() ) { jto["env-base"] = *manifest.envBase; }

  if ( manifest.options.has_value() ) { jto["options"] = *manifest.options; }

  if ( manifest.install.has_value() ) { jto["install"] = *manifest.install; }

  if ( manifest.registry.has_value() ) { jto["registry"] = *manifest.registry; }

  if ( manifest.vars.has_value() ) { jto["vars"] = *manifest.vars; }

  if ( manifest.hook.has_value() ) { jto["hook"] = *manifest.hook; }
}


/* -------------------------------------------------------------------------- */

void
ManifestRaw::check() const
{
  GlobalManifestRaw::check();
  if ( this->envBase.has_value() ) { this->envBase->check(); }
  if ( this->install.has_value() )
    {
      for ( const auto & [iid, desc] : *this->install ) { desc->check( iid ); }
    }
  if ( this->hook.has_value() ) { this->hook->check(); }
}


/* -------------------------------------------------------------------------- */

nlohmann::json
ManifestRaw::diff( const ManifestRaw & old ) const
{
  return nlohmann::json::diff( nlohmann::json( *this ), old );
}


/* -------------------------------------------------------------------------- */

}  // namespace flox::resolver


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
