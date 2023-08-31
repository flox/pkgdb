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
  export DBPATH="$( $PKGDB get db "$NIXPKGS_REF"; )";
}

# Dump parameters for a query on `nixpkgs'.
genParams() {
  jq '.query.match|=null' "$TDATA/params0.json"|jq "${1?}";
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:match

# Searches `nixpkgs#legacyPackages.x86_64-linux' for a fuzzy match on "hello"
@test "'pkgdb search' params0.json" {
  run $PKGDB search "$TDATA/params0.json";
  assert_success;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:opt
#
# FIXME: You need to add `--databases DIR' to this in order to really make this
# work, currently it's running off the user's homedir database which is
# convenient for development, but we can't assume that the developer hasn't
# tried to scrape that prefix in the past.

#@test "'pkgdb search' scrapes only named subtrees" {
#  run $PKGDB search "$TDATA/params0.json";
#  assert_success;
#  run $PKGDB get id "$DBPATH" x86_64-linux packages;
#  assert_failure;
#  assert_output "ERROR: No such AttrSet 'x86_64-linux.packages'.";
#}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:match
#
@test "'pkgdb search' 'match=hello'" {
  run sh -c "$PKGDB search '$TDATA/params0.json'|wc -l;";
  assert_success;
  assert_output 10;
  run sh -c "$PKGDB search '$TDATA/params0.json'|grep hello|wc -l;";
  assert_success;
  assert_output 10;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:pname

# Exact `pname' match
@test "'pkgdb search' 'pname=hello'" {
  run sh -c "$PKGDB search '$( genParams '.query.pname|="hello"'; )'|wc -l;";
  assert_success;
  assert_output 1;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:version

# Exact `version' match
@test "'pkgdb search' 'pname=nodejs & version=18.16.0'" {
  run sh -c "$PKGDB search '$(
    genParams '.query.pname|="nodejs"|.query.version="18.16.0"';
  )'|wc -l;";
  assert_success;
  assert_output 4;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:semver

# Test `semver' by filtering to >18.16, leaving only 20.2 and its alias.
@test "'pkgdb search' 'pname=nodejs & semver=>18.16.0'" {
  run sh -c "$PKGDB search '$(
    genParams '.query.pname|="nodejs"|.query.semver=">18.16.0"';
  )'|wc -l;";
  assert_success;
  assert_output 2;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:name

# Exact `name' match.
@test "'pkgdb search' 'name=nodejs-18.16.0'" {
  run sh -c "$PKGDB search '$(
    genParams '.query.name|="nodejs-18.16.0"';
  )'|wc -l;";
  assert_success;
  assert_output 4;
}


# ---------------------------------------------------------------------------- #

# TODO: unfree
# TODO: unbroken
# TODO: licenses
# TODO: preferPreReleases


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
