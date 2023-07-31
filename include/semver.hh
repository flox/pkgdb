/* ========================================================================== *
 *
 * @file semver.hh
 *
 * @brief Interfaces used to perform version number analysis, especially
 *        _Semantic Version_ processing.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <regex>
#include <optional>
#include <list>
#include <nix/util.hh>


/* -------------------------------------------------------------------------- */

/** Interfaces for analyzing version numbers */
namespace versions {

/* -------------------------------------------------------------------------- */

/** @return `true` iff @a version is a valid _semantic version_ string. */
bool isSemver( const std::string & version );
/** @return `true` iff @a version is a valid _semantic version_ string. */
bool isSemver( std::string_view version );

/** @return `true` iff @a version is a _datestamp-like_ version string. */
bool isDate( const std::string & version );
/** @return `true` iff @a version is a _datestamp-like_ version string. */
bool isDate( std::string_view version );

/** @return `true` iff @a version can be interpreted as _semantic version_. */
bool isCoercibleToSemver( const std::string & version );
/** @return `true` iff @a version can be interpreted as _semantic version_. */
bool isCoercibleToSemver( std::string_view version );

/* -------------------------------------------------------------------------- */

/**
 * Attempt to coerce strings such as `"v1.0.2"` or `1.0` to valid semantic
 * version strings.
 * @return `std::nullopt` iff @a version cannot be interpreted as
 *          _semantic version_.
 *          A valid semantic version string otherwise.
 */
std::optional<std::string> coerceSemver( std::string_view version );


/* -------------------------------------------------------------------------- */

/**
 * Invokes `node-semver` by `exec`.
 * @param args List of arguments to pass to `semver` executable.
 * @return Pair of error-code and output string.
 */
std::pair<int, std::string> runSemver( const std::list<std::string> & args );

/**
 * Filter a list of versions by a `node-semver` _semantic version range_.
 * @param range A _semantic version range_ as taken by `node-semver`.
 * @param versions A list of _semantic versions_ to filter.
 * @return The list of _semantic versions_ from @a versions which fall in the
 *         range specified by @a range.
 */
std::list<std::string> semverSat( const std::string            &  range
                                , const std::list<std::string> & versions
                                );


/* -------------------------------------------------------------------------- */

}  /* End namespace `versions' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
