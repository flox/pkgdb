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

  # We don't parallelize these to avoid DB sync headaches and to recycle the
  # cache between tests.
  # Nonetheless this file makes an effort to avoid depending on past state in
  # such a way that would make it difficult to eventually parallelize in
  # the future.
  export BATS_NO_PARALLELIZE_WITHIN_FILE=true;

  # Change the rev used for the `--ga-registry' flag to align with our cached
  # revision used by other tests.
  # This is both an optimization and a way to ensure consistency of test output.
  export _PKGDB_GA_REGISTRY_REF_OR_REV="$NIXPKGS_REV";
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:ga-registry

# Assert that the search command accepts the `--ga-registry' options.
@test "'pkgdb search --help' has '--ga-registry'" {
  run $PKGDB search --help;
  assert_success;
  assert_output --partial "--ga-registry";
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:ga-registry

# Ensure that the search command succeeds with the `--ga-registry' option and
# no other registry.
@test "'pkgdb search --ga-registry' provides 'global-manifest'" {
  run sh -c "$PKGDB search --ga-registry --pname hello|wc -l";
  assert_success;
  assert_output 1;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:ga-registry

# Ensure that the search command with `--ga-registry' option uses
# `_PKGDB_GA_REGISTRY_REF_OR_REV' as the `nixpkgs' revision.
@test "'pkgdb search --ga-registry' uses '_PKGDB_GA_REGISTRY_REF_OR_REV'" {
  run sh -c "$PKGDB search --ga-registry --pname hello -vv 2>&1 >/dev/null";
  assert_success;
  assert_output --partial "$NIXPKGS_REF";
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
