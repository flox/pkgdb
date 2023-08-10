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
  run sqlite3 "$DBPATH"                                               \
    "SELECT description FROM Descriptions WHERE id = (
      SELECT descriptionId FROM Packages
      WHERE attrName = 'unstable-2020-02-09'
    )";

  assert_output 'A foreign environment interface for Fish shell';
}


# ---------------------------------------------------------------------------- #

# This test should be updated when the Packages schema is changed.
@test "nixpkgs-flox fishPlugins.foreign-env check all fields" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"           \
                    catalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  VERSION='unstable-2020-02-09'
  # SEMVER foreign-env has a non-semantic version, so check if NULL below
  NAME='fishplugin-foreign-env-unstable-2020-02-09'
  # LICENSE catalogs entries don't currently set license, so check if NULL below
  OUTPUTS='["out"]'
  OUTPUTS_TO_INSTALL='["out"]'
  # BROKEN catalogs entries don't currently set broken, so check if NULL below
  UNFREE='0'
  run sqlite3 "$DBPATH" \
    "SELECT
      IIF(version = '$VERSION', '', 'version incorrect'),
      IIF(semver is NULL, '', 'semver incorrect'),
      IIF(name = '$NAME', '', 'name incorrect'),
      IIF(license is NULL, '', 'license incorrect'),
      IIF(outputs = '$OUTPUTS', '', 'outputs incorrect'),
      IIF(outputsToInstall = '$OUTPUTS_TO_INSTALL', '', 'outputsToInstall incorrect'),
      IIF(broken is NULL, '', 'broken incorrect'),
      IIF(unfree = '$UNFREE', '', 'unfree incorrect')
    FROM Packages
    WHERE attrName = 'unstable-2020-02-09'";
  assert_output '|||||||';
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
#
#
#
# ============================================================================ #
