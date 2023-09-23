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
#include "flox/resolver/manifest.hh"


/* -------------------------------------------------------------------------- */

using namespace nlohmann::literals;


/* -------------------------------------------------------------------------- */

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
  EXPECT( std::holds_alternative<nix::FlakeRef>( * descriptor.input ) );

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_getLockedInputs()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  flox::resolver::ManifestRaw raw = flox::yamlToJSON( yaml );
  flox::resolver::Manifest    manifest( raw );

  for ( const auto & [name, ref] : manifest.getLockedInputs() )
    {
      std::cerr << name << ": " << ref.to_string() << std::endl;
    }

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

  RUN_TEST( getLockedInputs );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
