# pkgdb search

The `pkgdb search` command may be used to search for packages which match a
set of `query` filters in a set of [registry](./registry.md) inputs.

This command accepts JSON input and emits JSON output, which are
described below.


## Input

`pkgdb` search accepts either a path to a JSON file or inline JSON with
the following abstract schema:

```json
System ::= "x86_64-linux" | "aarch64-linux" | "x86_64-darwin" | "aarch64-darwin"

License ::= ? SPDX License Identifier ?

Allows ::= {
  unfree   = true | false
, broken   = true | false
, licenses = null | [License...]
}

Semver ::= {
  preferPreReleases = true | false
}

SearchQuery ::= {
  name    = null | <STRING>
  pname   = null | <STRING>
  version = null | <STRING>
  semver  = null | <STRING>
  match   = null | <STRING>
}

SearchParams ::= {
  registry = Registry
, systems  = null | [System...]
, allow    = Allows
, semver   = Semver
, query    = SearchQuery
}
```

- `query.match` strings are used to test partial matches found in a package's
  `name`, `pname`, or `description` fields.
  + Exact matches cause search results to appear before ( higher ) than
    partial matches.
- `query.semver` strings are a
  [node-semver](https://github.com/npm/node-semver#ranges) range filter.
  + These use the exact syntax found in tools such as `npm`, `yarn`, `pip`, and
    many other package managers.
- `query.name`, `query.pname`, and `query.version` fields indicate an EXACT
  match on a derivation's `<DRV>.name`, `<DRV>.pname`, `<DRV>.version` field.
  + For derivations which lack `<DRV>.pname` and/or `<DRV>.version` fields,
    `builtins.parseDrvName` is used parse them from `<DRV>.name`.
- `allow.licenses` strings are those found in many _nixpkgs_
  `meta.license.spdxId` fields.
  + If `null` is used or this field is omitted, any license is allowed.
  + A complete list of SPDX Identifiers may be
    found [here](https://spdx.org/licenses/).
- `allow.broken` is associated with _nixpkgs_ `meta.broken` field.
  + The default value is `false` which causes any packages marked `broken` to
    be omitted from search results.
  + Derivations which lack this field are assumed to be _unbroken_, and will be
    unaffected by this setting.
- `allow.unfree` is associated with the _nixpkgs_ `meta.unfree` field.
  + The default value is `true` which allows _unfree_
    ( non-[FOSS](https://www.gnu.org/philosophy/floss-and-foss.en.html) ) to
    appear in search results.
  + Derivations which lack this field are assumed to be _free_, and will be
    unaffected by this setting.
- `semver.preferPreReleases` controls whether packages with _pre-release tags_
  before ( higher ) than untagged releases.
  + The default value is `false`, causing _pre-release tagged_ packages to
    appear after ( lower ) major _official_ release, so `4.1.9` will appear
    before `4.2.0-pre`.
  + Setting this to `true` causes search results sort _pre-release_ versions
    _normally_, so `4.2.0-pre` will appear before `4.1.9`.
  + Conventionally _pre-release tags_ are used to mark development or unstable
    builds, which may not be what users actually want to install/search for.
  + This setting only effects packages who have `<DRV>.version` strings which
    can be interpreted as semantic version strings.


Full details about the registry field may be found [here](./registry.md).
