/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <gtest/gtest.h>

#include "flox/core/util.hh"
#include "flox/resolver/descriptor.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from TOML to JSON. */
TEST( tomlToJSON, simple )
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.toml" );
  std::string   toml( ( std::istreambuf_iterator<char>( ifs ) ),
                    ( std::istreambuf_iterator<char>() ) );

  nlohmann::json manifest = flox::tomlToJSON( toml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>(),
             "Howdy" );
}


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from YAML to JSON. */
TEST( yamlToJSON, simple )
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                    ( std::istreambuf_iterator<char>() ) );

  nlohmann::json manifest = flox::yamlToJSON( yaml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>(),
             "Howdy" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test that a simple descriptor can be parsed from JSON. */
TEST( parseManifestDescriptor, simple )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "4.2.0"
  , "optional": true
  , "packageGroup": "blue"
  , "packageRepository": "nixpkgs"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.name.has_value() );
  EXPECT_EQ( *descriptor.name, "foo" );

  /* Ensure this string was detected as an _exact_ version match. */
  EXPECT_FALSE( descriptor.semver.has_value() );
  EXPECT_TRUE( descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.version, "4.2.0" );

  EXPECT_TRUE( descriptor.group.has_value() );
  EXPECT_EQ( *descriptor.group, "blue" );
  EXPECT_EQ( descriptor.optional, true );

  /* We expect this to be recognized as an _indirect flake reference_. */
  EXPECT_TRUE( descriptor.input.has_value() );
  EXPECT_TRUE( std::holds_alternative<nix::FlakeRef>( *descriptor.input ) );
  auto flakeRef = std::get<nix::FlakeRef>( *descriptor.input );

  EXPECT_EQ( flakeRef.input.getType(), "indirect" );

  auto alias = flakeRef.input.attrs.at( "id" );
  EXPECT_TRUE( std::holds_alternative<std::string>( alias ) );
  EXPECT_EQ( std::get<std::string>( alias ), "nixpkgs" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
TEST( parseManifestDescriptor, versionRange )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "^4.2.0"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of semver range. */
  EXPECT_FALSE( descriptor.version.has_value() );
  EXPECT_TRUE( descriptor.semver.has_value() );
  EXPECT_EQ( *descriptor.semver, "^4.2.0" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
TEST( parseManifestDescriptor, versionRangePartial )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "4.2"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of semver range. */
  EXPECT_FALSE( descriptor.version.has_value() );
  EXPECT_TRUE( descriptor.semver.has_value() );
  EXPECT_EQ( *descriptor.semver, "4.2" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
TEST( parseManifestDescriptor, versionExact )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "=4.2"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of exact version match.
   * Ensure the leading `=` is stripped. */
  EXPECT_FALSE( descriptor.semver.has_value() );
  EXPECT_TRUE( descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.version, "4.2" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
TEST( parseManifestDescriptor, versionEmpty )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": ""
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection glob/_any_ version match. */
  EXPECT_TRUE( descriptor.semver.has_value() );
  EXPECT_FALSE( descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.semver, "" );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing inline inputs. */
TEST( parseManifestDescriptor, inlineInputAttrs )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "packageRepository": {
      "type": "github"
    , "owner": "NixOS"
    , "repo": "nixpkgs"
    }
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.input.has_value() );
  EXPECT_TRUE( std::holds_alternative<nix::FlakeRef>( *descriptor.input ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing inline inputs. */
TEST( parseManifestDescriptor, inlineInputString )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "input": "./pkgs/foo/default.nix"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.input.has_value() );
  EXPECT_TRUE( std::holds_alternative<std::string>( *descriptor.input ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathNull )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": "legacyPackages.null.hello"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT_FALSE( descriptor.systems.has_value() );
  EXPECT_FALSE( descriptor.stability.has_value() );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathStar )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": "legacyPackages.*.hello"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT_FALSE( descriptor.systems.has_value() );
  EXPECT_FALSE( descriptor.stability.has_value() );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathListNull )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", null, "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT_FALSE( descriptor.systems.has_value() );
  EXPECT_FALSE( descriptor.stability.has_value() );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathListStar )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", "*", "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT_FALSE( descriptor.systems.has_value() );
  EXPECT_FALSE( descriptor.stability.has_value() );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathList )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", "x86_64-linux", "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT_TRUE( descriptor.systems.has_value() );
  EXPECT_TRUE( ( *descriptor.systems )
          == ( std::vector<std::string> { "x86_64-linux" } ) );
  EXPECT_FALSE( descriptor.stability.has_value() );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
TEST( parseManifestDescriptor, absPathCatalog )
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["catalog", "x86_64-linux", "stable", "hello", "4.2.0"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_TRUE( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_CATALOG );
  EXPECT_TRUE( descriptor.systems.has_value() );
  EXPECT_TRUE( ( *descriptor.systems )
          == ( std::vector<std::string> { "x86_64-linux" } ) );
  EXPECT_TRUE( descriptor.stability.has_value() );
  EXPECT_EQ( *descriptor.stability, "stable" );
  EXPECT_TRUE( descriptor.path.has_value() );
  EXPECT_TRUE( ( *descriptor.path ) == ( flox::AttrPath { "hello", "4.2.0" } ) );
}


/* -------------------------------------------------------------------------- */

int
main( int argc, char * argv[] )
{
  testing::InitGoogleTest( & argc, argv );
  testing::TestEventListeners & listeners =
    testing::UnitTest::GetInstance()->listeners();
  /* Delete the default listener. */
  delete listeners.Release( listeners.default_result_printer() );
  listeners.Append( new tap::TapListener() );
  return RUN_ALL_TESTS();
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
