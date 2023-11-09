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

    # Add IWYU pragmas
    overlays.nlohmann = final: prev: {
      nlohmann_json = final.callPackage ./pkgs/nlohmann_json.nix {
        inherit (prev) nlohmann_json;
      };
    };

    # Use nix@2.17
    overlays.nix = final: prev: {
      nix = final.callPackage ./pkgs/nix/pkg-fun.nix {};
    };

    # Aggregate dependency overlays.
    overlays.deps = nixpkgs.lib.composeManyExtensions [
      overlays.nlohmann
      floco.overlays.default
      overlays.nix
      sqlite3pp.overlays.default
      argparse.overlays.default
    ];

    overlays.flox-pkgdb = final: prev: {
      flox-pkgdb = final.callPackage ./pkg-fun.nix {};
    };

    overlays.default = nixpkgs.lib.composeExtensions overlays.deps
                                                     overlays.flox-pkgdb;


# ---------------------------------------------------------------------------- #

    # Apply overlays to the `nixpkgs` _base_ set.
    # This is exposed as an output later; but we don't use the name
    # `legacyPackages' to avoid checking the full closure with
    # `nix flake check' and `nix search'.
    pkgsFor = eachDefaultSystemMap ( system: let
      base = builtins.getAttr system nixpkgs.legacyPackages;
    in base.extend overlays.default );


# ---------------------------------------------------------------------------- #

    packages = eachDefaultSystemMap ( system: let
      pkgs = builtins.getAttr system pkgsFor;
    in {
      inherit (pkgs) flox-pkgdb semver nix;
      default = pkgs.flox-pkgdb;
      pkgdb   = pkgs.flox-pkgdb;
    } );


# ---------------------------------------------------------------------------- #

  in {

    inherit overlays packages pkgsFor;

    devShells = eachDefaultSystemMap ( system: let
      pkgs = builtins.getAttr system pkgsFor;
      flox-pkgdb-shell = let
        batsWith = pkgs.bats.withLibraries ( libs: [
          libs.bats-assert
          libs.bats-file
          libs.bats-support
        ] );
      in pkgs.mkShell {
        name       = "flox-pkgdb-shell";
        inputsFrom = [pkgs.flox-pkgdb];
        packages   = [
          # For tests
          batsWith
          pkgs.jq
          # For profiling
          pkgs.lcov
          ( if pkgs.stdenv.cc.isGNU or false then pkgs.gdb else
            pkgs.lldb
          )
          # For doc
          pkgs.doxygen
          # For IDEs
          pkgs.ccls
          pkgs.bear
          # For lints/fmt
          pkgs.clang-tools_16
          # For debugging
        ] ++ (
          if pkgs.stdenv.isLinux or false then [pkgs.valgrind] else []
        );
        inherit (pkgs.flox-pkgdb)
          nix_INCDIR boost_CFLAGS toml_CFLAGS yaml_PREFIX libExt SEMVER_PATH
        ;
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
