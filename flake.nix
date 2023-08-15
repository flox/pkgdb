# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{

# ---------------------------------------------------------------------------- #

  inputs.nixpkgs.url                      = "github:NixOS/nixpkgs";
  inputs.floco.url                        = "github:aakropotkin/floco";
  inputs.floco.inputs.nixpkgs.follows     = "/nixpkgs";
  inputs.argparse.url                     = "github:aakropotkin/argparse";
  inputs.argparse.inputs.nixpkgs.follows  = "/nixpkgs";
  inputs.sqlite3pp.url                    = "github:aakropotkin/sqlite3pp";
  inputs.sqlite3pp.inputs.nixpkgs.follows = "/nixpkgs";


# ---------------------------------------------------------------------------- #

  outputs = { nixpkgs, floco, argparse, sqlite3pp, ... }: let

# ---------------------------------------------------------------------------- #

    eachDefaultSystemMap = let
      defaultSystems = [
        "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"
      ];
    in fn: let
      proc = system: { name = system; value = fn system; };
    in builtins.listToAttrs ( map proc defaultSystems );


# ---------------------------------------------------------------------------- #

    overlays.deps = nixpkgs.lib.composeManyExtensions [
      floco.overlays.default
      sqlite3pp.overlays.default
      argparse.overlays.default
    ];
    overlays.flox-pkgdb = final: prev: {
      flox-pkgdb = final.callPackage ./pkg-fun.nix {};
    };
    overlays.default = nixpkgs.lib.composeExtensions overlays.deps
                                                     overlays.flox-pkgdb;


# ---------------------------------------------------------------------------- #

    packages = eachDefaultSystemMap ( system: let
      pkgsFor = ( builtins.getAttr system nixpkgs.legacyPackages ).extend
                  overlays.default;
    in {
      inherit (pkgsFor) flox-pkgdb;
      default = pkgsFor.flox-pkgdb;
    } );


# ---------------------------------------------------------------------------- #

  in {

    inherit overlays packages;

    devShells = eachDefaultSystemMap ( system: let
      pkgsFor = ( builtins.getAttr system nixpkgs.legacyPackages ).extend
                  overlays.default;
      flox-pkgdb-shell = let
        batsWith = pkgsFor.bats.withLibraries ( libs: [
          libs.bats-assert
          libs.bats-file
          libs.bats-support
        ] );
      in pkgsFor.mkShell {
        name       = "flox-pkgdb-shell";
        inputsFrom = [pkgsFor.flox-pkgdb];
        packages   = [
          # For tests
          batsWith
          pkgsFor.jq
          # For profiling
          pkgsFor.lcov
          ( if pkgsFor.stdenv.cc.isGNU then pkgsFor.gdb else pkgsFor.lldb )
          # For doc
          pkgsFor.doxygen
        ] ++ nixpkgs.lib.optionals pkgsFor.stdenv.isLinux [
          # For debugging
          pkgsFor.valgrind
        ];
        inherit (pkgsFor.flox-pkgdb) nix_INCDIR libExt SEMVER_PATH;
        shellHook = ''
          shopt -s autocd;

          alias gs='git status';
          alias ga='git add';
          alias gc='git commit -am';
          alias gl='git pull';
          alias gp='git push';

          {
            echo "";
            echo "Build with \`make' ( or \`make -j8' to go fast )";
            echo "";
            echo "Run with \`./bin/pkgdb --help'";
            echo "";
            echo "Test with \`make check'";
            echo "";
            echo "Read docs with: \`make docs && firefox ./docs/index.hml'";
            echo "";
            echo "See more tips in \`CONTRIBUTING.md'";
          } >&2;
        '';
      };
    in {
      inherit flox-pkgdb-shell;
      default = flox-pkgdb-shell;
    } );

  };


# ---------------------------------------------------------------------------- #


}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
