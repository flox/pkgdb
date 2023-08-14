#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# Tests for
# `NIXPKGS_FLOX_REF#catalog.{x86_64-linux,aarch64-darwin}.stable.fishPlugins'
# All assertions are made about `fishPlugins.foreign-env.unstable-2020-02-09'.
#
# I couldn't find an attrSet in the catalog for all of `x86_64-linux',
# `aarch64-darwin', and `x86_64-darwin', so hardcode system in these
# tests instead.
#
# I unsuccessfully attempted to find such an attrSet by loading the stable
# stability of all 3 systems into a database and then running the following
# query.
# It may or may not be accurate but it only came back with stable so I gave up.
# SELECT grandparentName, count(*) as numberOfSystems
# FROM (
#   -- look for grandparent since in the catalog each package name is an attrSet
#   SELECT (
#     SELECT attrName
#     FROM AttrSets
#     WHERE id = (SELECT parent FROM AttrSets WHERE id = parentId)
#   ) as grandparentName,
#   count(*) as systemSpecificCount
#   FROM Packages
#   -- group by grandparent id
#   GROUP BY (SELECT parent FROM AttrSets WHERE id = parentId)
# )
# -- there could be the same grandparent name in distinct attribute paths, but in
# -- that case numberOfSystems will be artificially high and only provide false
# -- positives
# GROUP BY grandparentName
# HAVING numberOfSystems >= 3
#
#
# ---------------------------------------------------------------------------- #

load setup_suite.bash;

# bats file_tags=cli,scrape,catalog


# ---------------------------------------------------------------------------- #

setup_file() {
  export DBPATH="$BATS_FILE_TMPDIR/test.sqlite";
  mkdir -p "$BATS_FILE_TMPDIR";
  # We don't parallelize these to avoid DB sync headaches and to recycle the
  # cache between tests.
  # Nonetheless this file makes an effort to avoid depending on past state in
  # such a way that would make it difficult to eventually parallelize in
  # the future.
  export BATS_NO_PARALLELIZE_WITHIN_FILE=true;
}


# ---------------------------------------------------------------------------- #

@test "fishPlugins.foreign-env description" {
  for SYSTEM in 'x86_64-linux' 'aarch64-darwin'; do
    run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"  \
                      catalog "$SYSTEM" stable fishPlugins;
    assert_success;
  done
  run sqlite3 "$DBPATH"                                 \
    "SELECT description FROM Descriptions WHERE id = (
      SELECT descriptionId FROM Packages
      WHERE attrName = 'unstable-2020-02-09'
    )";

  assert_output 'A foreign environment interface for Fish shell';
}


# ---------------------------------------------------------------------------- #

# This test should be updated when the Packages schema is changed.
@test "fishPlugins.foreign-env check all fields" {
  for SYSTEM in 'x86_64-linux' 'aarch64-darwin'; do
    run $PKGDB scrape --database "$DBPATH" "$NIXPKGS_FLOX_REF"  \
                      catalog "$SYSTEM" stable fishPlugins;
    assert_success;
  done

  VERSION='unstable-2020-02-09'
  # SEMVER foreign-env has a non-semantic version, so check if NULL below
  PNAME='foreign-env'
  NAME='fishplugin-foreign-env-unstable-2020-02-09'
  # LICENSE catalogs entries don't currently set license, so check if NULL below
  OUTPUTS='["out"]'
  OUTPUTS_TO_INSTALL='["out"]'
  # BROKEN catalogs entries don't currently set broken, so check if NULL below
  UNFREE='0'
  # this query should select a row for each system
  run sqlite3 "$DBPATH" \
    "SELECT
      IIF(version = '$VERSION', '', 'version incorrect'),
      IIF(semver is NULL, '', 'semver incorrect'),
      IIF(pname is '$PNAME', '', 'pname incorrect'),
      IIF(name = '$NAME', '', 'name incorrect'),
      IIF(license is NULL, '', 'license incorrect'),
      IIF(outputs = '$OUTPUTS', '', 'outputs incorrect'),
      IIF(outputsToInstall = '$OUTPUTS_TO_INSTALL', '',
          'outputsToInstall incorrect'),
      IIF(broken is NULL, '', 'broken incorrect'),
      IIF(unfree = '$UNFREE', '', 'unfree incorrect')
    FROM Packages
    WHERE attrName = 'unstable-2020-02-09'";
  # two sets of |||||||
  assert_output --regexp '\|{7}.*\|{7}';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
