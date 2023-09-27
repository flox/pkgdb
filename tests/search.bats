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
  export PKGDB_CACHEDIR="$BATS_FILE_TMPDIR/pkgdbs";
  echo "PKGDB_CACHEDIR: $PKGDB_CACHEDIR" >&3;
  # We don't parallelize these to avoid DB sync headaches and to recycle the
  # cache between tests.
  # Nonetheless this file makes an effort to avoid depending on past state in
  # such a way that would make it difficult to eventually parallelize in
  # the future.
  export BATS_NO_PARALLELIZE_WITHIN_FILE=true;
}

# Dump parameters for a query on `nixpkgs'.
genParams() {
  jq -r '.query.match|=null' "$TDATA/params0.json"|jq "${1?}";
}

genParamsNixpkgsFlox() {
  jq -r '.query.match|=null
        |.registry.inputs|=( del( .nixpkgs )|del( .floco ) )'  \
     "$TDATA/params1.json"|jq "${1?}";
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

@test "'pkgdb search' scrapes only named subtrees" {
  DBPATH="$( $PKGDB get db "$NIXPKGS_REF"; )";
  run $PKGDB search "$TDATA/params0.json";
  assert_success;
  run $PKGDB get id "$DBPATH" x86_64-linux packages;
  assert_failure;
}


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

# bats test_tags=search:version, search:pname

# Exact `version' match
@test "'pkgdb search' 'pname=nodejs & version=18.16.0'" {
  run sh -c "$PKGDB search '$(
    genParams '.query.pname|="nodejs"|.query.version="18.16.0"';
  )'|wc -l;";
  assert_success;
  assert_output 4;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:semver, search:pname

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

# bats test_tags=search:name, search:license

# Licenses filter
@test "'pkgdb search' 'pname=blobs.gg & licenses=[Apache-2.0]'" {
  run sh -c "$PKGDB search '$(
    genParams '.query.pname|="blobs.gg"|.allow.licenses|=["Apache-2.0"]';
  )'|wc -l;";
  assert_success;
  assert_output 1;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:unfree

# Unfree filter
@test "'pkgdb search' 'allow.unfree=false'" {
  run sh -c "$PKGDB search '$( genParams '.allow.unfree=false'; )'|wc -l;";
  assert_success;
  assert_output 61338;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:broken

# Unfree filter
@test "'pkgdb search' 'allow.broken=true'" {
  run sh -c "$PKGDB search '$( genParams '.allow.broken=true'; )'|wc -l;";
  assert_success;
  assert_output 64037;
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:prerelease, search:pname

# preferPreReleases ordering
@test "'pkgdb search' 'semver.preferPreReleases=true'" {
  run sh -c "$PKGDB search '$(
    genParams '.semver.preferPreReleases=true|.query.pname="zfs-kernel"';
  )'|head -n1|jq -r .version;";
  assert_success;
  assert_output '2.1.12-staging-2023-04-18-6.1.31';
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:system, search:pname

# `systems' ordering
@test "'pkgdb search' systems order" {
  run sh -c "$PKGDB search '$(
    genParams '.systems=["x86_64-linux","x86_64-darwin"]|.query.pname="hello"';
  )'|jq -rs 'to_entries|map( ( .key|tostring ) + \" \" + .value.path[1] )[]'";
  assert_success;
  assert_output --partial '0 x86_64-linux';
  assert_output --partial '1 x86_64-darwin';
  refute_output --partial '2 ';
}


# bats test_tags=search:system, search:pname

# `systems' ordering, reverse order of previous
@test "'pkgdb search' systems order ( reversed )" {
  run sh -c "$PKGDB search '$(
    genParams '.systems=["x86_64-darwin","x86_64-linux"]|.query.pname="hello"';
  )'|jq -rs 'to_entries|map( ( .key|tostring ) + \" \" + .value.path[1] )[]'";
  assert_success;
  assert_output --partial '0 x86_64-darwin';
  assert_output --partial '1 x86_64-linux';
  refute_output --partial '2 ';
}


# ---------------------------------------------------------------------------- #

# bats test_tags=search:stabilities, search:pname

# `stabilities' ordering
@test "'pkgdb search' stabilities order" {
  run sh -c "$PKGDB search -qq '$( genParamsNixpkgsFlox          \
    '.registry.inputs["nixpkgs-flox"].stabilities+=["unstable"]
    |.query.pname|="hello"
    |.query.version|="2.12.1"';
  )'|jq -rs 'to_entries|map( ( .key|tostring ) + \" \" + .value.path[2] )[]'";
  assert_success;
  # catalog.x86_64-linux.stable.hello.2_12_1
  assert_output --partial '0 stable';
  # catalog.x86_64-linux.stable.hello.latest
  assert_output --partial '1 stable';
  # catalog.x86_64-linux.unstable.hello.2_12_1
  assert_output --partial '2 unstable';
  # catalog.x86_64-linux.unstable.hello.latest
  assert_output --partial '3 unstable';
  refute_output --partial '4 ';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
