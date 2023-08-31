#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# `pkgdb search' tests.
#
# ---------------------------------------------------------------------------- #

load setup_suite.bash;

# bats file_tags=search

setup_file() {
  export TDATA="$TESTS_DIR/data/search";
}


# ---------------------------------------------------------------------------- #

# Searches `nixpkgs#legacyPackages.x86_64-linux' for a fuzzy match on "hello"
@test "'pkgdb search' params0.json" {
  run $PKGDB search "$TDATA/params0.json";
  assert_success;
}


# ---------------------------------------------------------------------------- #

@test "'pkgdb search' scrapes only named subtrees" {
  run $PKGDB search "$TDATA/params0.json";
  assert_success;
  DBPATH="$( $PKGDB get db "$NIXPKGS_REF"; )";
  run $PKGDB get id "$DBPATH" x86_64-linux packages;
  assert_failure;
  assert_output "ERROR: No such AttrSet 'x86_64-linux.packages'.";
}


# ---------------------------------------------------------------------------- #

@test "'pkgdb search' 'match=hello'" {
  run sh -c "$PKGDB search '$TDATA/params0.json'|wc -l;";
  assert_success;
  assert_output 10;
  run sh -c "$PKGDB search '$TDATA/params0.json'|grep hello|wc -l;";
  assert_success;
  assert_output 10;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
