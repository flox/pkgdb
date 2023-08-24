/* ========================================================================== *
 *
 * @file versions.hh
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

/** Typed exception wrapper used for version parsing/comparison errors. */
class VersionException : public std::exception {
  private:
    std::string msg;
  public:
    VersionException( std::string_view msg ) : msg( msg ) {}
    const char * what() const noexcept override { return this->msg.c_str(); }
};


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

enum version_kind { VK_NONE = 0, VK_OTHER = 1, VK_DATE = 2, VK_SEMVER = 3 };

  static inline version_kind
getVersionKind( const std::string & version )
{
  return isSemver( version ) ? VK_SEMVER :
         isDate( version )   ? VK_DATE   : VK_OTHER;
}


/* -------------------------------------------------------------------------- */

/**
 * Compare two semantic version strings.
 * No coercion is attempted, any coercion to a semantic version must be
 * performed before attempting a comparison.
 *
 * @param lhs A semantic version string.
 * @param rhs A semantic version string.
 * @param preferPreReleases Whether versions with pre-release tags should be
 *                          sorted as _less than_ major releases.
 * @return `true` iff @a lhs should be sorted before @a rhs.
 */
bool compareSemVersLT( const std::string & lhs
                     , const std::string & rhs
                     ,       bool          preferPreReleases = false
                     );


/* -------------------------------------------------------------------------- */

/**
 * Compare two date version strings of the format `%Y-%m-%d`.
 * Any trailing characters will be used to break ties by
 * sorting lexicographically.
 *
 * @param lhs A date version string.
 * @param rhs A date version string.
 * @return `true` iff @a lhs should be sorted before @a rhs.
 */
bool compareDateVersLT( const std::string & lhs, const std::string & rhs );


/* -------------------------------------------------------------------------- */

/**
 * Compare two version strings.
 * No coercion is attempted, any coercion to a semantic version must be
 * performed before attempting a comparison.
 *
 * Semantic versions are sorted accoring to semantic version standards.
 * Date-like versions are compared as dates.
 * Any other type of versions are compared lexicographically.
 * When @a lhs and @a rhs are not of the same _category_ of version string,
 * sorting is performed on the categories themselves such that
 * other < date-like < semver.
 *
 * @param lhs A version string.
 * @param rhs A version string.
 * @param preferPreReleases Whether versions with pre-release tags should be
 *                          sorted as _less than_ major releases.
 * @return `true` iff @a lhs should be sorted before @a rhs.
 */
bool compareVersionsLT( const std::string & lhs
                      , const std::string & rhs
                      ,       bool          preferPreReleases = false
                      );


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
