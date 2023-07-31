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
, boost
, argparse
, semver
, sql-builder
, sqlite3pp
}: stdenv.mkDerivation {
  pname   = "flox-pkgdb";
  version = "0.1.0";
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
        "lib"
      ];
      notIgnored = ! ( builtins.elem bname ignores );
      notObject  = ( builtins.match ".*\\.o" name ) == null;
      notResult  = ( builtins.match "result(-*)?" bname ) == null;
      isSrc      = ( builtins.match ".*\\.cc" name ) != null;
    in notIgnored && notObject && notResult && (
      ( ( dirOf name ) == "tests" ) -> isSrc
    );
  };
  nativeBuildInputs = [pkg-config];
  buildInputs       = [
    sqlite.dev nlohmann_json nix.dev boost argparse sqlite3pp
  ];
  propagatedBuildInputs = [semver];
  nix_INCDIR            = nix.dev.outPath + "/include";
  boost_CFLAGS          = "-I" + boost.outPath + "/include";
  libExt                = stdenv.hostPlatform.extensions.sharedLibrary;
  SEMVER_PATH           = semver.outPath + "/bin/semver";
  sql_builder_CFLAGS    = "-I" + sql-builder.outPath + "/include";
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
