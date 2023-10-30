# pkgdb search

The `pkgdb search` command may be used to search for packages which match a
set of `query` filters in a set of [registry](./registry.md) inputs.

This command accepts JSON input and emits JSON output, which are
described below.


## Input

`pkgdb` search accepts either a path to a JSON file or inline JSON with
the following abstract schema:

```
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
  `name`, `pname`, or `description` fields as well as partial matches on a
  calculated column `attrName`.
  + `attrName` is _the last attribute path element_ for `packages` and
     `legacyPackages` subtrees.
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
- The `systems` field controls which systems will appear in results.
  + If omitted or `null`, only the _current_ system will be searched.


Full details about the registry field may be found [here](./registry.md).


### Example Query

Below is an example query that searches four flakes for
_any package matching the string "hello"_ with _major version 2_ , usable
on `x86_64-linux`.

```json
{
  "registry": {
    "inputs": {
      "nixpkgs": {
        "from": {
          "type": "github"
        , "owner": "NixOS"
        , "repo": "nixpkgs"
        , "rev": "e8039594435c68eb4f780f3e9bf3972a7399c4b1"
        }
      , "subtrees": ["legacyPackages"]
      }
    , "floco": { "from": "github:aakropotkin/floco" }
  , "defaults": {
      "subtrees": ["packages"]
    }
  , "priority": ["nixpkgs"]
  }
, "systems": ["x86_64-linux"]
, "query": { "match": "hello", "semver": "2" }
}
```

In the example above we'll make a few observations to clarify how defaults are
applied to `inputs` by showing the _explicit_ form of the same params:

```json
{
  "registry": {
    "inputs": {
      "nixpkgs": {
        "from": {
          "type": "github"
        , "owner": "NixOS"
        , "repo": "nixpkgs"
        , "rev": "e8039594435c68eb4f780f3e9bf3972a7399c4b1"
        }
      , "subtrees": ["legacyPackages"]
      }
    , "floco": {
        "from": {
          "type": "github"
        , "owner": "aakropotkin"
        , "repo": "floco"
        , "rev": "1e84b4b16bba5746e1195fa3a4d8addaaf2d9ef4"
        }
        , "subtrees": ["packages"]
      }
    }
  , "defaults": {
      "subtrees": ["packages"]
    }
  , "priority": ["nixpkgs", "floco"]
  }
, "systems": ["x86_64-linux"]
, "allow": {
    "unfree": true
  , "broken": false
  , "licenses": null
  }
, "semver": {
    "preferPreReleases": false
  }
, "query": {
    "name": null
  , "pname": null
  , "version": null
  , "semver": "2"
  , "match": "hello"
  }
}
```

- `inputs.<NAME>.subtrees` was applied to all inputs which didn't explicitly
   specify them.
- `inputs.<NAME>.from` _flake references_ were parsed and locked.
- `allow` and `semver` fields were added with their default values.
- `priority` list added _missing_ inputs in lexicographical order.
  + in this case _lexicographical_ is _alphabetical_ because there are no
    numbers or symbols in the names.
- Missing `query.*` fields were filled with `null`.


## Output

Search results are printed as JSON objects with one result per line ordered
such that _high ranking_ results appear **before** _low ranking_ results.

This single result per line format is printed in chunks as each input is
processed, and is suitable for _streaming_ across a pipe.
Tools such as `jq` or `sed` may be used in combination with `pkgdb search` so
that results are displayed to users _as they are processed_.


Each output line has the following format:

```
Result ::= {
  id           = <INT>
, input        = <INPUT-NAME>
, subtree      = "packages" | "legacyPackages"
, absPath      = [<STRING>...]
, relPath   = [<STRING>...]
, pname        = <STRING>
, version      = null | <STRING>
, description  = null | <STRING>
, license      = null | License
, broken       = true | false
, unfree       = true | false
}
```

Note that because the `input` field only prints the short-name of its input,
it is **strongly recommended** that the caller use _locked flake references_.


### Example Output

For the example query parameters given above, we get the following results:

```json
{"absPath":["legacyPackages","x86_64-linux","hello"],"broken":false,"description":"A program that produces a familiar, friendly greeting","id":6095,"input":"nixpkgs","license":"GPL-3.0-or-later","pname":"hello","relPath":["hello"],"subtree":"legacyPackages","system":"x86_64-linux","unfree":false,"version":"2.12.1"}
```
