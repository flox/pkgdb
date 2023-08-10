#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# Tests for NIXPKGS_FLOX_REF#catalog.$NIX_SYSTEM.stable.fishPlugins
# All assertions are made about fishPlugins.foreign-env.unstable-2020-02-09
#
# ---------------------------------------------------------------------------- #

load setup_suite.bash;

# bats file_tags=cli


# ---------------------------------------------------------------------------- #

setup_file() {
  export DBPATH="$BATS_FILE_TMPDIR/test-cli.sqlite";
  mkdir -p "$BATS_FILE_TMPDIR";
  # We don't parallelize these to avoid DB sync headaches and to recycle the
  # cache between tests.
  # Nonetheless this file makes an effort to avoid depending on past state in
  # such a way that would make it difficult to eventually parallelize in
  # the future.
  export BATS_NO_PARALLELIZE_WITHIN_FILE=true;
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env description" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins foreign-env;
  local _dID;
  _dID="$(
    sqlite3 "$DBPATH"                                       \
    "SELECT descriptionId FROM Packages  \
     WHERE attrName = 'unstable-2020-02-09'";
  )";
  run sqlite3 "$DBPATH"                                               \
    "SELECT description FROM Descriptions WHERE id = $_dID";
  assert_output 'A foreign environment interface for Fish shell';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env version" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT version FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output 'unstable-2020-02-09';
}


# ---------------------------------------------------------------------------- #

# foreign-env has a non-semantic version.
@test "nixpkgs-flox fishPlugins.foreign-env semver" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT semver FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env pname" {
  skip FIXME;
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT pname FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output 'foreign-env';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env name" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT name FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output 'fishplugin-foreign-env-unstable-2020-02-09';
}


# ---------------------------------------------------------------------------- #

# catalogs entries don't currently set license
@test "nixpkgs-flox fishPlugins.foreign-env license" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT license FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env outputs" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT outputs FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env outputsToInstall" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT outputsToInstall FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

# catalogs entries don't currently set broken
@test "nixpkgs-flox fishPlugins.foreign-env broken" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT broken FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env unfree" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH" "SELECT unfree FROM Packages      \
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output '0';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
