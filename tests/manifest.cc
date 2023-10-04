/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include "test.hh"
#include "flox/core/util.hh"
#include "flox/resolver/descriptor.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from TOML to JSON. */
  bool
test_tomlToJSON0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.toml" );
  std::string   toml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  nlohmann::json manifest = flox::tomlToJSON( toml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>()
           , "Howdy"
           );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief test the conversion of an example manifest from YAML to JSON. */
  bool
test_yamlToJSON0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  nlohmann::json manifest = flox::yamlToJSON( yaml );

  EXPECT_EQ( manifest.at( "vars" ).at( "message" ).get<std::string>()
           , "Howdy"
           );

  return true;
}


/* -------------------------------------------------------------------------- */

/** @brief Test that a simple descriptor can be parsed from JSON. */
  bool
test_parseManifestDescriptor0()
{

  flox::resolver::ManifestDescriptorRaw raw = R"( {
    "name": "foo"
  , "version": "1.2.3"
  , "optional": true
  , "packageGroup": "blue"
  , "packageRepository": "nixpkgs"
  } )"_json;

  flox::resolver::ManifestDescriptor descriptor( raw );

  EXPECT_EQ( * descriptor.name, "foo" );
  EXPECT_EQ( * descriptor.version, "1.2.3" );
  EXPECT_EQ( * descriptor.group, "blue" );
  EXPECT_EQ( descriptor.optional, true );

  /* We expect this to be recognized as an _indirect flake reference_. */
  EXPECT( std::holds_alternative<nix::FlakeRef>( * descriptor.input ) );
  auto flakeRef = std::get<nix::FlakeRef>( * descriptor.input );

  EXPECT_EQ( flakeRef.input.getType(), "indirect" );

  auto alias = flakeRef.input.attrs.at( "id" );
  EXPECT( std::holds_alternative<std::string>( alias ) );
  EXPECT_EQ( std::get<std::string>( alias ), "nixpkgs" );

  return true;
}


/* -------------------------------------------------------------------------- */

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( tomlToJSON0 );

  RUN_TEST( yamlToJSON0 );

  RUN_TEST( parseManifestDescriptor0 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
