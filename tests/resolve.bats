#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# `pkgdb resolve' tests.
#
# ---------------------------------------------------------------------------- #

load setup_suite.bash;

# bats file_tags=resolve

setup_file() {
  export TDATA="$TESTS_DIR/data/resolve";
  export PKGDB_CACHEDIR="$BATS_FILE_TMPDIR/pkgdbs";
  echo "PKGDB_CACHEDIR: $PKGDB_CACHEDIR" >&3;
  # We don't parallelize these to avoid DB sync headaches and to recycle the
  # cache between tests.
  # Nonetheless this file makes an effort to avoid depending on past state in
  # such a way that would make it difficult to eventually parallelize in
  # the future.
  export BATS_NO_PARALLELIZE_WITHIN_FILE=true;
}

# Dump parameters for a query
genParams() {
  jq -r '.query.pname|=null|.query.semver=null'  \
        "$TDATA/params0.json"|jq "${1?}";
}


# ---------------------------------------------------------------------------- #

# bats test_tags=resolve:pname, resolve:semver

# Resolves pname="hello" & semver = ">=2"
#@test "'pkgdb resolve' params0.json" {
#  run $PKGDB resolve "$TDATA/params0.json";
#  assert_success;
#}


# ---------------------------------------------------------------------------- #

# bats test_tags=resolve:pname

# Exact `pname' match
@test "'pkgdb resolve' 'pname=hello'" {
  run sh -c "$PKGDB resolve '$(
    genParams '.query.pname|="hello"';
  )' 2>/dev/null|jq -r 'length';";
  assert_success;
  assert_output 13;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
