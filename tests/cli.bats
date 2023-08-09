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
  export DBPATH_NIXPKGS_FLOX="$BATS_FILE_TMPDIR/test-nixpkgs-flox.sqlite";
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
  assert_output 'Blob emoji from blobs.gg repacked as APNG';
}


# ---------------------------------------------------------------------------- #

# Check the version of a package.
@test "akkoma-emoji version" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT version FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output 'unstable-2019-07-24';
}


# ---------------------------------------------------------------------------- #

# Check the semver of a package with a non-semantic version.
@test "akkoma-emoji semver" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT semver FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

# Check the pname of a package.
@test "akkoma-emoji pname" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT pname FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output 'blobs.gg';
}


# ---------------------------------------------------------------------------- #

# Check the attribute name of a package
@test "akkoma-emoji attrName" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT attrName FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output 'blobs_gg';
}


# ---------------------------------------------------------------------------- #

# Check the license of a package
@test "akkoma-emoji license" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT license FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output 'Apache-2.0';
}


# ---------------------------------------------------------------------------- #

# Check the outputs of a package
@test "akkoma-emoji outputs" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT outputs FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

# Check the outputs to install from a package
@test "akkoma-emoji outputsToInstall" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT outputsToInstall FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

# Check whether a package is broken
@test "akkoma-emoji broken" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT broken FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output '0';
}


# ---------------------------------------------------------------------------- #

# Check whether a package is unfree
@test "akkoma-emoji unfree" {
  run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_REF"           \
                    legacyPackages "$NIX_SYSTEM" 'akkoma-emoji';
  assert_success;
  run sqlite3 "$DBPATH" "SELECT unfree FROM Packages      \
    WHERE name = 'blobs.gg-unstable-2019-07-24' LIMIT 1";
  assert_output '0';
}

# ---------------------------------------------------------------------------- #
#
# Tests for NIXPKGS_FLOX_REF#evalCatalog.$NIX_SYSTEM.stable.fishPlugins
# All assertions are made about the foreign-env package.
#
# ============================================================================ #

# ---------------------------------------------------------------------------- #

# Check the description of a package.
@test "nixpkgs-flox fishPlugins.foreign-env description" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  local _dID;
  _dID="$(
    sqlite3 "$DBPATH_NIXPKGS_FLOX"                                       \
    "SELECT descriptionId FROM Packages  \
     WHERE attrName = 'foreign-env'";
  )";
  run sqlite3 "$DBPATH_NIXPKGS_FLOX"                                               \
    "SELECT description FROM Descriptions WHERE id = $_dID";
  assert_output 'A foreign environment interface for Fish shell';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env version" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT version FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output 'unstable-2020-02-09';
}


# ---------------------------------------------------------------------------- #

# Check the semver of a package with a non-semantic version.
@test "nixpkgs-flox fishPlugins.foreign-env semver" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT semver FROM Packages      \
    WHERE attrName = 'foreign-env'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

# Check the pname of a package.
@test "nixpkgs-flox fishPlugins.foreign-env pname" {
  skip FIXME;
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT pname FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output 'foreign-env';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env name" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT name FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output 'fishplugin-foreign-env-unstable-2020-02-09';
}


# ---------------------------------------------------------------------------- #

# catalogs entries don't currently set license
@test "nixpkgs-flox fishPlugins.foreign-env license" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT license FROM Packages      \
    WHERE attrName = 'foreign-env'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env outputs" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT outputs FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

@test "nixpkgs-flox fishPlugins.foreign-env outputsToInstall" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT outputsToInstall FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output '["out"]';
}


# ---------------------------------------------------------------------------- #

# catalogs entries don't currently set broken
@test "nixpkgs-flox fishPlugins.foreign-env broken" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT broken FROM Packages      \
    WHERE attrName = 'foreign-env'";
  refute_output --regexp '.';
}


# ---------------------------------------------------------------------------- #

# Check whether a package is unfree
@test "nixpkgs-flox fishPlugins.foreign-env unfree" {
  run $PKGDB scrape --database "$DBPATH_NIXPKGS_FLOX" "$NIXPKGS_FLOX_REF"           \
                    evalCatalog "$NIX_SYSTEM" stable fishPlugins;
  assert_success;
  run sqlite3 "$DBPATH_NIXPKGS_FLOX" "SELECT unfree FROM Packages      \
    WHERE attrName = 'foreign-env'";
  assert_output '0';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
