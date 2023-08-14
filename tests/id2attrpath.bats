#! /usr/bin/env bats
# -*- mode: bats; -*-
# ============================================================================ #
#
# `id2attrpath' executable tests.
#
# ---------------------------------------------------------------------------- #

load setup_suite.bash;

# bats file_tags=id2attrpath


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
  export TEST_HARNESS_FLAKE="$TESTS_DIR/harnesses/proj0";
  # Load a database to query during testing
  "$PKGDB" scrape --database "$DBPATH" "$TEST_HARNESS_FLAKE" packages "$NIX_SYSTEM";
}


# ---------------------------------------------------------------------------- #

@test "retrieves existing attrset" {
    run "$ID2ATTRPATH" "$DBPATH" 2;
    assert_output "packages $NIX_SYSTEM";
}


# ---------------------------------------------------------------------------- #

@test "error on nonexistent attrset" {
    run "$ID2ATTRPATH" "$DBPATH" 3;
    assert_failure;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
