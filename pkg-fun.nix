# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ stdenv
, sqlite
, pkg-config
, nlohmann_json
, nix
, argparse
, semver
, sqlite3pp
}: stdenv.mkDerivation {
  pname   = "flox-pkgdb";
  version = builtins.replaceStrings ["\n"] [""] ( builtins.readFile ./version );
  src     = builtins.path {
    path = ./.;
    filter = name: type: let
      bname   = baseNameOf name;
      ignores = [
        "default.nix"
        "pkg-fun.nix"
        "flake.nix"
        "flake.lock"
        ".ccls"
        ".ccls-cache"
        ".git"
        ".gitignore"
        "out"
        "bin"
      ];
      notIgnored   = ! ( builtins.elem bname ignores );
      notResult    = ( builtins.match "result(-*)?" bname ) == null;
      notObject    = ( builtins.match ".*\\.o" name ) == null;
      notSharedLib = ( builtins.match ".*\\.so" name ) == null;
      notDyLib     = ( builtins.match ".*\\.dylib" name ) == null;
      isSrc        = ( builtins.match ".*\\.cc" name ) != null;
      isBats       = ( builtins.match ".*\\.bats" name ) != null;
      isBash       = ( builtins.match ".*\\.bash" name ) != null;
    in notIgnored && notObject && notSharedLib && notDyLib && notResult && (
      ( ( dirOf name ) == "tests" ) -> ( isSrc || isBats || isBash )
    );
  };
  nativeBuildInputs = [pkg-config];
  buildInputs       = [sqlite.dev nlohmann_json nix.dev argparse sqlite3pp];
  propagatedBuildInputs = [semver];
  nix_INCDIR            = nix.dev.outPath + "/include";
  libExt                = stdenv.hostPlatform.extensions.sharedLibrary;
  SEMVER_PATH           = semver.outPath + "/bin/semver";
  configurePhase        = ''
    runHook preConfigure;
    export PREFIX="$out";
    if [[ "''${enableParallelBuilding:-1}" = 1 ]]; then
      makeFlagsArray+=( '-j4' );
    fi
    runHook postConfigure;
  '';
  # Checks require internet
  doCheck        = false;
  doInstallCheck = false;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
