# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ stdenv
, clang16Stdenv  # For Darwin
, sqlite
, pkg-config
, nlohmann_json
, nix
, boost
, argparse
, semver
, sqlite3pp
}: let
  pkgdbStdenv = if ( stdenv.hostPlatform.useLLVM or false ) ||
                   ( stdenv.hostPlatform.isDarwin or false )
                then clang16Stdenv
                else stdenv;
in pkgdbStdenv.mkDerivation {
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
      notIgnored  = ( ! ( builtins.elem bname ignores ) ) &&
                    ( ! ( builtins.elem ext ignoredExts ) );
      notResult = ( builtins.match "result(-*)?" bname ) == null;
    in notIgnored && notResult;
  };
  propagatedBuildInputs = [semver nix.dev boost];
  nativeBuildInputs     = [pkg-config];
  buildInputs           = [sqlite.dev nlohmann_json argparse sqlite3pp];
  nix_INCDIR            = nix.dev.outPath + "/include";
  boost_CFLAGS          = "-I" + boost.outPath + "/include";
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
