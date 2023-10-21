/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <iostream>
#include <cstddef>

#define GTEST_TAP_PRINT_TO_STDOUT 1
#include "tap.hh"


/* -------------------------------------------------------------------------- */

/* This shouldn't happen, but it's a sane fallback for running from the
 * project root. */
#ifndef TEST_DATA_DIR
#  define TEST_DATA_DIR  "./tests/data"
#endif  /* End `ifndef TEST_DATA_DIR' */


/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";

/**
 * These counts indicate the total number of derivations under
 * `<nixpkgsRef>#legacyPackages.x86_64-linux.**' which we will use to sanity
 * check calls to `size()'.
 * Note that the legacy implementation used to populate `DbPackageSet' will
 * fail to evaluate 3 packages which require `NIXPKGS_ALLOW_BROKEN', causing
 * different sizes to be collected ( until migration is coompleted ).
 */
static const size_t unbrokenPkgCount = 64037;
static const size_t fullPkgCount     = 64040;


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
