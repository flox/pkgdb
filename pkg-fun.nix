# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ stdenv
, sqlite
, pkg-config
, nlohmann_json
, nixVersions
, boost
, argparse
, semver
, sqlite3pp
, toml11
, yaml-cpp
}: let
  nix = nixVersions.nix_2_15;
in
  stdenv.mkDerivation {
  pname   = "flox-pkgdb";
  version = builtins.replaceStrings ["\n"] [""] ( builtins.readFile ./version );
  src     = builtins.path {
    path = ./.;
    filter = name: type: let
      bname   = baseNameOf name;
      ignores = [
        "default.nix" "pkg-fun.nix" "flake.nix" "flake.lock"
        ".ccls" ".ccls-cache"
        ".git" ".gitignore"
        "out" "bin"
        "tests"  # Tests require internet so there's no point in including them
      ];
      ext = let
        m = builtins.match ".*\\.([^.]+)" name;
      in if m == null then "" else builtins.head m;
      ignoredExts = ["o" "so" "dylib"];
      notResult   = ( builtins.match "result(-*)?" bname ) == null;
      notIgnored  = ( ! ( builtins.elem bname ignores ) ) &&
                    ( ! ( builtins.elem ext ignoredExts ) );
    in notIgnored && notResult;
  };
  propagatedBuildInputs = [semver nix];
  nativeBuildInputs     = [pkg-config];
  buildInputs           = [
    sqlite.dev nlohmann_json argparse sqlite3pp toml11 yaml-cpp boost nix
  ];
  nix_INCDIR     = nix.dev.outPath + "/include";
  boost_CFLAGS   = "-I" + boost.dev.outPath + "/include";
  toml_CFLAGS    = "-I" + toml11.outPath + "/include";
  yaml_PREFIX    = yaml-cpp.outPath;
  libExt         = stdenv.hostPlatform.extensions.sharedLibrary;
  SEMVER_PATH    = semver.outPath + "/bin/semver";
  configurePhase = ''
    runHook preConfigure;
    export PREFIX="$out";
    if [[ "''${enableParallelBuilding:-1}" = 1 ]]; then
      makeFlagsArray+=( '-j4' );
    fi
    runHook postConfigure;
  '';
  # Checks require internet
  doCheck          = false;
  doInstallCheck   = false;
  outputs = ["out" "dev"];
  meta.mainProgram = "pkgdb";
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
