# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ nixpkgs         ? builtins.getFlake "nixpkgs"
, floco           ? builtins.getFlake "github:aakropotkin/floco"
, sqlite3pp-flake ? builtins.getFlake "github:aakropotkin/sqlite3pp"
, sql-builder-src ? builtins.fetchTree {
                    type = "github"; owner = "six-ddc"; repo = "sql-builder";
                  }
, system        ? builtins.currentSystem
, pkgsFor       ? nixpkgs.legacyPackages.${system}
, stdenv        ? pkgsFor.stdenv
, sqlite        ? pkgsFor.sqlite
, pkg-config    ? pkgsFor.pkg-config
, nlohmann_json ? pkgsFor.nlohmann_json
, nix           ? pkgsFor.nix
, boost         ? pkgsFor.boost
, argparse      ? pkgsFor.argparse
, semver        ? floco.packages.${system}.semver
, sqlite3pp     ? sqlite3pp-flake.packages.${system}.sqlite3pp
, sql-builder   ? pkgsFor.runCommandNoCC "sql-builder" {
                    src = sql-builder-src;
                  } ''
                    mkdir -p "$out/include/sql-builder";
                    cp "$src/sql.h" "$out/include/sql-builder/sql.hh";
                  ''
}: import ./pkg-fun.nix {
  inherit
    stdenv pkg-config nlohmann_json nix boost argparse semver
    sqlite sqlite3pp sql-builder
  ;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
