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
        "default.nix" "pkg-fun.nix" "flake.nix" "flake.lock"
        ".ccls" ".ccls-cache"
        ".git" ".gitignore"
        "out" "bin"
      ];
      ext = let
        m = builtins.match ".*\\.([^.]+)" name;
      in if m == null then "" else builtins.head m;
      ignoredExts = ["o" "so" "dylib"];
      notIgnored  = ( ! ( builtins.elem bname ignores ) ) &&
                    ( ! ( builtins.elem ext ignoredExts ) );
      notResult = ( builtins.match "result(-*)?" bname ) == null;
      testsKeep = builtins.elem ext ["cc" "bats" "bash"];
    in notIgnored && notResult &&
       ( ( ( dirOf name ) == "tests" ) -> testsKeep );
  };
  nativeBuildInputs     = [pkg-config];
  buildInputs           = [sqlite.dev nlohmann_json nix.dev argparse sqlite3pp];
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
