# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{

# ---------------------------------------------------------------------------- #

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/release-23.05";

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

    /* Use nix@2.15 */
    overlays.nix = final: prev: { nix = prev.nixVersions.nix_2_15; };
    /* Aggregate dependency overlays. */
    overlays.deps = nixpkgs.lib.composeManyExtensions [
      overlays.nix
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

      ciPkgs = let
        batsWith = pkgsFor.bats.withLibraries ( libs: [
          libs.bats-assert
          libs.bats-file
          libs.bats-support
        ] );
      in [
        # For tests
        batsWith
        pkgsFor.jq
        # For doc
        pkgsFor.doxygen
      ];

      # Used for interactive development
      devPkgs = [
        # For profiling
        pkgsFor.lcov
        ( if pkgsFor.stdenv.cc.isGNU or false then pkgsFor.gdb else
          pkgsFor.lldb
        )
        # For IDEs
        pkgsFor.ccls
        pkgsFor.bear
        # For lints/fmt
        pkgsFor.clang-tools_16
        # For debugging
      ] ++ (
        if pkgsFor.stdenv.isLinux or false then [pkgsFor.valgrind] else []
      );

      shellEnv = {
        name       = "pkgdb-shell";
        inputsFrom = [pkgsFor.flox-pkgdb];
        inherit packages;
        inherit (pkgsFor.flox-pkgdb)
          nix_INCDIR boost_CFLAGS toml_CFLAGS yaml_PREFIX libExt SEMVER_PATH
        ;
      };

      pkgdb-shell = pkgsFor.mkShell ( shellEnv // {
          packages = ciPkgs ++ devPkgs;
          shellHook = ''
            shopt -s autocd;

            alias gs='git status';
            alias ga='git add';
            alias gc='git commit -am';
            alias gl='git pull';
            alias gp='git push';

            if [ -z "''${NO_WELCOME:-}" ]; then
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
            fi
          '';
        } );

    in {
      inherit pkgdb-shell;
      default = pkgdb-shell;
      ci      = pkgsFor.mkShell ( shellEnv // { packages = ciPkgs; });
    } );

  };


# ---------------------------------------------------------------------------- #


}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
