#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# `pkgdb' CLI tests.
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

@test "pkgdb --help" {
  run $PKGDB --help;
  assert_success;
}


# ---------------------------------------------------------------------------- #

@test "pkgdb scrape --help" {
  run $PKGDB scrape --help;
  assert_success;
}


# ---------------------------------------------------------------------------- #

# This attrset only contains a single package so it's a quick run.
@test "pkgdb scrape <NIXPKGS> legacyPackages $NIX_SYSTEM akkoma-emoji" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
}


# ---------------------------------------------------------------------------- #

# Check the description of a package.
@test "akkoma-emoji description" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  local _dID;
  _dID="$(
    sqlite3 "$DBPATH"                                       \
    "SELECT descriptionId FROM Packages  \
     WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  )";
  assert test "$_dID" = 1;
  run sqlite3 "$DBPATH"                                               \
    "SELECT description FROM Descriptions WHERE id = $_dID LIMIT 1";
  assert_output --partial 'Blob emoji from blobs.gg repacked as APNG';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
