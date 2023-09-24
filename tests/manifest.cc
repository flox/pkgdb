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
#include "flox/resolver/resolve.hh"


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
test_getRegistryRaw0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  flox::resolver::ManifestRaw raw = flox::yamlToJSON( yaml );
  flox::resolver::Manifest    manifest( raw );

  auto registry = manifest.getRegistryRaw();

  EXPECT_EQ( manifest.getInlineInputs().size(), std::size_t( 1 ) );

  bool found = false;
  for ( const auto & [name, input] : registry.inputs )
    {
      if ( name == "__inline__python3" ) { found = true; break; }
    }

  EXPECT( found );

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_getLockedInputs0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  flox::resolver::ManifestRaw raw = flox::yamlToJSON( yaml );
  flox::resolver::Manifest    manifest( raw );

  auto locked = manifest.getLockedInputs();

  EXPECT( locked.find( "__inline__python3" ) != locked.end() );

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_getDescriptorGroups0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  flox::resolver::ManifestRaw raw = flox::yamlToJSON( yaml );
  flox::resolver::Manifest    manifest( raw );

  (void) manifest.getDescriptorGroups();

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_resolveDescriptor0()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );
  flox::resolver::ManifestRaw raw = flox::yamlToJSON( yaml );
  flox::resolver::Manifest    manifest( raw );
  auto resolutions = manifest.resolveDescriptor( "hello" );
  EXPECT( ! resolutions.empty() );

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_resolveDescriptor1()
{
  std::ifstream ifs( TEST_DATA_DIR "/manifest/manifest0.yaml" );
  std::string   yaml( ( std::istreambuf_iterator<char>( ifs ) ),
                      ( std::istreambuf_iterator<char>() )
                    );

  using namespace flox::resolver;

  ManifestRaw raw = flox::yamlToJSON( yaml );
  Manifest    manifest( raw );


  auto resolutions = manifest.resolveDescriptor( "python3" );

  EXPECT_EQ( resolutions.size(), std::size_t( 17 ) );

  // FIXME: remove JSON output, this was for debugging.
  nlohmann::json jresolutions = nlohmann::json::array();

  for ( const auto & resolution : resolutions )
    {
      EXPECT_EQ( resolution.input
               , "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1"
               );
      jresolutions.push_back( nlohmann::json {
        { "input", resolution.input }
      , { "path",  resolution.path  }
      } );
    }

  std::cout << jresolutions.dump() << std::endl;

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

  RUN_TEST( getRegistryRaw0 );

  RUN_TEST( getLockedInputs0 );

  RUN_TEST( getDescriptorGroups0 );

  RUN_TEST( resolveDescriptor0 );
  RUN_TEST( resolveDescriptor1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
