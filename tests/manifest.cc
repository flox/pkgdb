/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "flox/core/util.hh"
#include "flox/resolver/descriptor.hh"
#include "test.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from TOML to JSON. */
bool
test_tomlToJSON0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.toml" );
  std::string   toml( ( std::istreambuf_iterator<char>( ifs ) ),
                    ( std::istreambuf_iterator<char>() ) );

  nlohmann::json manifest = flox::tomlToJSON( toml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>(),
             "Howdy" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from YAML to JSON. */
bool
test_yamlToJSON0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                    ( std::istreambuf_iterator<char>() ) );

  nlohmann::json manifest = flox::yamlToJSON( yaml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>(),
             "Howdy" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test that a simple descriptor can be parsed from JSON. */
bool
test_parseManifestDescriptor0()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "4.2.0"
  , "optional": true
  , "packageGroup": "blue"
  , "packageRepository": "nixpkgs"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.name.has_value() );
  EXPECT_EQ( *descriptor.name, "foo" );

  /* Ensure this string was detected as an _exact_ version match. */
  EXPECT( ! descriptor.semver.has_value() );
  EXPECT( descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.version, "4.2.0" );

  EXPECT( descriptor.group.has_value() );
  EXPECT_EQ( *descriptor.group, "blue" );
  EXPECT_EQ( descriptor.optional, true );

  /* We expect this to be recognized as an _indirect flake reference_. */
  EXPECT( descriptor.input.has_value() );
  EXPECT( std::holds_alternative<nix::FlakeRef>( *descriptor.input ) );
  auto flakeRef = std::get<nix::FlakeRef>( *descriptor.input );

  EXPECT_EQ( flakeRef.input.getType(), "indirect" );

  auto alias = flakeRef.input.attrs.at( "id" );
  EXPECT( std::holds_alternative<std::string>( alias ) );
  EXPECT_EQ( std::get<std::string>( alias ), "nixpkgs" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
bool
test_parseManifestDescriptor_version0()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "^4.2.0"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of semver range. */
  EXPECT( ! descriptor.version.has_value() );
  EXPECT( descriptor.semver.has_value() );
  EXPECT_EQ( *descriptor.semver, "^4.2.0" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
bool
test_parseManifestDescriptor_version1()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "4.2"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of semver range. */
  EXPECT( ! descriptor.version.has_value() );
  EXPECT( descriptor.semver.has_value() );
  EXPECT_EQ( *descriptor.semver, "4.2" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
bool
test_parseManifestDescriptor_version2()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "=4.2"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection of exact version match.
   * Ensure the leading `=` is stripped. */
  EXPECT( ! descriptor.semver.has_value() );
  EXPECT( descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.version, "4.2" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing of semver ranges and version matches. */
bool
test_parseManifestDescriptor_version3()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": ""
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  /* Expect detection glob/_any_ version match. */
  EXPECT( descriptor.semver.has_value() );
  EXPECT( ! descriptor.version.has_value() );
  EXPECT_EQ( *descriptor.semver, "" );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing inline inputs. */
bool
test_parseManifestDescriptor_input0()
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

  EXPECT( descriptor.input.has_value() );
  EXPECT( std::holds_alternative<nix::FlakeRef>( *descriptor.input ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor parsing inline inputs. */
bool
test_parseManifestDescriptor_input1()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "input": "./pkgs/foo/default.nix"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.input.has_value() );
  EXPECT( std::holds_alternative<std::string>( *descriptor.input ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path0()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": "legacyPackages.null.hello"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT( ! descriptor.systems.has_value() );
  EXPECT( ! descriptor.stability.has_value() );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path1()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": "legacyPackages.*.hello"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT( ! descriptor.systems.has_value() );
  EXPECT( ! descriptor.stability.has_value() );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path2()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", null, "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT( ! descriptor.systems.has_value() );
  EXPECT( ! descriptor.stability.has_value() );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path3()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", "*", "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT( ! descriptor.systems.has_value() );
  EXPECT( ! descriptor.stability.has_value() );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path4()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["legacyPackages", "x86_64-linux", "hello"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_LEGACY );
  EXPECT( descriptor.systems.has_value() );
  EXPECT( ( *descriptor.systems ) ==
          ( std::vector<std::string> { "x86_64-linux" } ) );
  EXPECT( ! descriptor.stability.has_value() );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test descriptor `path`/`absPath` parsing. */
bool
test_parseManifestDescriptor_path5()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "absPath": ["catalog", "x86_64-linux", "stable", "hello", "4.2.0"]
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT( descriptor.subtree.has_value() );
  EXPECT_EQ( *descriptor.subtree, flox::ST_CATALOG );
  EXPECT( descriptor.systems.has_value() );
  EXPECT( ( *descriptor.systems ) ==
          ( std::vector<std::string> { "x86_64-linux" } ) );
  EXPECT( descriptor.stability.has_value() );
  EXPECT_EQ( *descriptor.stability, "stable" );
  EXPECT( descriptor.path.has_value() );
  EXPECT( ( *descriptor.path ) == ( flox::AttrPath { "hello", "4.2.0" } ) );

  return true;
}


/* -------------------------------------------------------------------------- */

int
main()
{
  int exitCode = EXIT_SUCCESS;
  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RUN_TEST( ... ) _RUN_TEST( exitCode, __VA_ARGS__ )

  RUN_TEST( tomlToJSON0 );

  RUN_TEST( yamlToJSON0 );

  RUN_TEST( parseManifestDescriptor0 );

  RUN_TEST( parseManifestDescriptor_version0 );
  RUN_TEST( parseManifestDescriptor_version1 );
  RUN_TEST( parseManifestDescriptor_version2 );
  RUN_TEST( parseManifestDescriptor_version3 );

  RUN_TEST( parseManifestDescriptor_input0 );
  RUN_TEST( parseManifestDescriptor_input1 );

  RUN_TEST( parseManifestDescriptor_path0 );
  RUN_TEST( parseManifestDescriptor_path1 );
  RUN_TEST( parseManifestDescriptor_path2 );
  RUN_TEST( parseManifestDescriptor_path3 );
  RUN_TEST( parseManifestDescriptor_path4 );
  RUN_TEST( parseManifestDescriptor_path5 );

  return exitCode;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
