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

    # Cherry pick `semver' recipe from `floco'.
    overlays.semver = final: prev: {
      semver = let
        base = final.callPackage "${floco}/fpkgs/semver" {
          nixpkgs = throw ( "`nixpkgs' should not be references when `pkgsFor' "
                            + "is provided"
                          );
          inherit (final) lib;
          pkgsFor = final;
          nodePackage = final.nodejs;
        };
      in base.overrideAttrs ( prevAttrs: {
        copyTree         = true;
        preferLocalBuild = false;
        allowSubstitutes = true;
        # Remove symlinks.
        postInstall = ''
          NMDIR="$out/lib/node_modules/semver/node_modules";
          chmod -R u+w "$NMDIR";
          todo="$( readlink -f "$NMDIR/lru-cache"; )";
          rm "$NMDIR/lru-cache";
          cp -rp "$todo" "$NMDIR/lru-cache";
          chmod -R u+w "$NMDIR/lru-cache";
          todo="$( readlink -f "$NMDIR/lru-cache/node_modules/yallist"; )";
          rm "$NMDIR/lru-cache/node_modules/yallist";
          cp -rp "$todo" "$NMDIR/lru-cache/node_modules/yallist";
        '';
      } );
    };

    # Aggregate dependency overlays.
    overlays.deps = nixpkgs.lib.composeManyExtensions [
      overlays.nlohmann
      overlays.semver
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

      # For use in GitHub Actions and local development.
      ciPkgs = let
        batsWith = pkgs.bats.withLibraries ( libs: [
          libs.bats-assert
          libs.bats-file
          libs.bats-support
        ] );
      in [
        # For tests
        batsWith
        pkgs.jq
        # For doc
        pkgs.doxygen
      ];

      # For use in local development.
      interactivePkgs = [
        # For profiling
        pkgs.lcov
        # For IDEs
        pkgs.ccls
        pkgs.bear
        # For lints/fmt
        pkgs.clang-tools_16
        pkgs.include-what-you-use
        # For debugging
        ( if pkgs.stdenv.cc.isGNU or false then pkgs.gdb else pkgs.lldb )
      ] ++ ( if pkgs.stdenv.isLinux or false then [pkgs.valgrind] else [] );

      mkPkgdbShell = name: packages: pkgs.mkShell {
        inherit name packages;
        inputsFrom = [pkgs.flox-pkgdb];
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
              echo "Build with \`make' ( or \`make -j' to go fast )";
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

      ci    = mkPkgdbShell "ci" ciPkgs;
      pkgdb = mkPkgdbShell "pkgdb" ( ciPkgs ++ interactivePkgs );

    in {
      inherit ci pkgdb;
      default = pkgdb;
    } );

  };


# ---------------------------------------------------------------------------- #


}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
