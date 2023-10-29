# Query Arguments -> SQL Query Pipeline

`pkgdb` exposes a number of interfaces for running complex queries against its
underlying collection of package databases, making them easy for callers to use.
These interfaces also help ensure consistent behavior and ranking between
_search_ and _resolution_ queries.

In order the ensure the behaviors of these two categories of queries are
consistent they share a common underlying _query builder_ called a
[PkgQuery](../include/flox/pkgdb/pkg-query.hh).
This class takes a set of filter rules provided by the caller, called
`PkgQueryArgs`, and can either emit a SQL `SELECT` statement as a string or
run that query on databases using `PkgQuery::bind( flox::pkgdb::database & )` or
`PkgQuery::execute( flox::pkgdb::database & )`.

Throughout this codebase you will find various `struct` and `class` definitions
which essentially exist to parse user input from JSON, TOML, or YAML and convert
them into `PkgQueryArgs` and `Registry<PkgDbInput>` _registries_; just remember
that past the tangled boilerplate of those routines - we're just moving fields
from the user's input, into these `PkgQueryArgs` or a set of `PkgDbInput`s.
This document will help you navigate these various structures and help you
identify common patterns among them.


## Query Argument Data

### PkgQueryArgs

This is the _finalized_ set of arguments used to actually query a database.
It has the following fields which are translated into SQL query filters,
and in the case of the `semver` field additional filtering using `node-semver`
will be performed.

```c++
// Taken from <pkgdb>/include/flox/pkgdb/pkg-query.hh
// NOTE: This document may be out of sync with `pkg-query.hh'.
//       The header itself is the _source of truth_.

struct PkgQueryArgs : public PkgDescriptorBase
{

  /* From `PkgDescriptorBase':
   *   std::optional<std::string> name;    //< Filter results by exact `name`.
   *   std::optional<std::string> pname;   //< Filter results by exact `pname`.
   *   std::optional<std::string> version; //< Filter results by exact version.
   *   std::optional<std::string> semver;  //< Filter results by version range.
   */

  /** Filter results by partial match on pname, pkgAttrName, or description */
  std::optional<std::string> partialMatch;

  /**
   * Filter results by an exact match on either `pname` or `pkgAttrName`.
   * To match just `pname` see @a flox::pkgdb::PkgDescriptorBase.
   */
  std::optional<std::string> pnameOrPkgAttrName;

  /** 
   * Filter results to those explicitly marked with the given licenses.
   *
   * NOTE: License strings should be SPDX Ids ( short names ).
   */
  std::optional<std::vector<std::string>> licenses;

  /** Whether to include packages which are explicitly marked `broken`. */
  bool allowBroken = false;

  /** Whether to include packages which are explicitly marked `unfree`. */
  bool allowUnfree = true;

  /** Whether pre-release versions should be ordered before releases. */
  bool preferPreReleases = false;

  /** 
   * Subtrees to search.
   * 
   * NOTE: `Subtree` is an enum of top level flake outputs, being one of
   * `"catalog"`, `"packages"`, or `"legacyPackages"`.
   */
  std::optional<std::vector<Subtree>> subtrees;

  /** Systems to search. Defaults to the current system. */
  std::vector<std::string> systems = { nix::settings.thisSystem.get() };

  /** 
   * Stabilities to search ( if any ).
   *
   * NOTE: Stabilities must be one of `"stable"`, `"staging"`, or `"unstable"`.
   */
  std::optional<std::vector<std::string>> stabilities;

  /**
   * Relative attribute path to package from its prefix.
   * For catalogs this is the part following `stability`, and for regular flakes
   * it is the part following `system`.
   *
   * NOTE: @a flox::AttrPath is an alias of `std::vector<std::string>`.
   */
  std::optional<flox::AttrPath> relPath;
  
  // ...<SNIP>...
  
};
```
