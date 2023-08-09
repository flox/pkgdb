# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{

# ---------------------------------------------------------------------------- #

  description = "A simple project used for testing";


# ---------------------------------------------------------------------------- #

  inputs.nixpkgs = {
    type  = "github";
    owner = "NixOS";
    repo  = "nixpkgs";
    # Keep this rev aligned with `<pkgdb>/tests/test.hh'
    rev = "e8039594435c68eb4f780f3e9bf3972a7399c4b1";
  };


# ---------------------------------------------------------------------------- #

  outputs = { nixpkgs, ... }: let


# ---------------------------------------------------------------------------- #

    eachDefaultSystemMap = let
      defaultSystems = [
        "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"
      ];
    in fn: let
      proc = system: { name = system; value = fn system; };
    in builtins.listToAttrs ( map proc defaultSystems );


# ---------------------------------------------------------------------------- #

    overlays.deps  = final: prev: {};
    overlays.proj0 = final: prev: {

      pkg0 = final.stdenv.mkDerivation {
        name         = "pkg";
        buildCommand = "mkdir -p \"$out\";";
      };

      pkg1 = final.stdenv.mkDerivation {
        pname            = "pkg";
        version          = "1";
        buildCommand     = "mkdir -p \"$out\";";
        meta.description = "So descriptive";
      };

      pkg2 = final.stdenv.mkDerivation {
        name         = "pkg-2";
        buildCommand = "mkdir -p \"$out\";";
        meta.license = 3;      # Bad type
        meta.broken  = false;
        meta.unfree  = false;  # unfree without license
      };

      pkg3 = final.stdenv.mkDerivation {
        pname        = "pkg";
        version      = "2023-08-09";
        buildCommand = "mkdir -p \"$out\";";
        meta.license = final.lib.licenses.mit;
        meta.broken  = true;
      };

      pkg4 = derivation {
        inherit (final) system;
        name    = "pkg";
        builder = final.bash.outPath + "/bin/bash";
        PATH    = final.coreutils.outPath + "/bin";
        args    = ["-eu" "-o" "pipefail" "-c" "mkdir -p \"$out\";"];
      };

    };
    overlays.default = nixpkgs.lib.composeExtensions overlays.deps
                                                     overlays.proj0;


# ---------------------------------------------------------------------------- #

  in {

    packages = eachDefaultSystemMap ( system: let
      pkgsFor = ( builtins.getAttr system nixpkgs.legacyPackages ).extend
                  overlays.default;
    in {
      inherit (pkgsFor) pkg0 pkg1 pkg2 pkg3 pkg4;
      default = pkgsFor.pkg0;
    } );

  };


# ---------------------------------------------------------------------------- #

}

# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
