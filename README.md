# Flox Resolver

Resolve Nix packages from flakes and catalogs from a set of requirements.

## Quick Start

```
Usage: resolver [-h] [--one] [--quiet] [--inputs INPUTS] [--preferences PREFERENCES] --descriptor DESCRIPTOR

Resolve nix package descriptors in flakes

Optional arguments:
  -h, --help                    shows help message and exits
  -v, --version                 prints version information and exits
  -o, --one                     return single resolved entry or `null'
  -q, --quiet                   exit 0 even if no resolutions are found
  -i, --inputs INPUTS           inline JSON or path to JSON file containing flake references [default: "{"nixpkgs":"github:NixOS/nixpkgs","nixpkgs-flox":"github:flox/nixpkgs-flox","floxpkgs":"github:flox/floxpkgs"}"]
  -p, --preferences PREFERENCES inline JSON or path to JSON file containing resolver preferences [default: "{}"]
  -d, --descriptor DESCRIPTOR   inline JSON or path to JSON file containing a package descriptor [required]
```

### Example Usage
``` shell
$ ./bin/resolver -i '{"nixpkgs":"github:NixOS/nixpkgs"}' -d '{"name":"hello","version":"2.12.1"}'|jq;
[
  {
    "info": {
      "aarch64-darwin": {
        "broken": false,
        "license": "GPL-3.0-or-later",
        "name": "hello-2.12.1",
        "outputs": [
          "out"
        ],
        "outputsToInstall": [
          "out"
        ],
        "pname": "hello",
        "semver": "2.12.1",
        "unfree": false,
        "version": "2.12.1"
      },
      "aarch64-linux": {
        "broken": false,
        "license": "GPL-3.0-or-later",
        "name": "hello-2.12.1",
        "outputs": [
          "out"
        ],
        "outputsToInstall": [
          "out"
        ],
        "pname": "hello",
        "semver": "2.12.1",
        "unfree": false,
        "version": "2.12.1"
      },
      "x86_64-darwin": {
        "broken": false,
        "license": "GPL-3.0-or-later",
        "name": "hello-2.12.1",
        "outputs": [
          "out"
        ],
        "outputsToInstall": [
          "out"
        ],
        "pname": "hello",
        "semver": "2.12.1",
        "unfree": false,
        "version": "2.12.1"
      },
      "x86_64-linux": {
        "broken": false,
        "license": "GPL-3.0-or-later",
        "name": "hello-2.12.1",
        "outputs": [
          "out"
        ],
        "outputsToInstall": [
          "out"
        ],
        "pname": "hello",
        "semver": "2.12.1",
        "unfree": false,
        "version": "2.12.1"
      }
    },
    "input": {
      "id": "nixpkgs",
      "locked": {
        "lastModified": 1688471249,
        "narHash": "sha256-urwh856+C1LlYFICB+yYOkDI5/1TAnYnedvu/oWtAD4=",
        "owner": "NixOS",
        "repo": "nixpkgs",
        "rev": "ac66b27438a74b2ab2b16b5b9f429b3049398383",
        "type": "github"
      }
    },
    "path": [
      "legacyPackages",
      null,
      "hello"
    ],
    "uri": "github:NixOS/nixpkgs/ac66b27438a74b2ab2b16b5b9f429b3049398383#legacyPackages.{{system}}.hello"
  }
]
```


## Descriptors

A _descriptor_ is a set of requirements written by the user describing a
dependency that they need.
This is similar to a search query and is used to filter a larger set of
packages down to a satisfactory set.

The simplest for of descriptor is likely a package name, or an attribute
path to a package.
This resolver features a number of additional filters which may be used to
describe packages.

### Fields

- `input`: restricts resolution to the named input.
- `name`: matches package `pname`, parsed `name`, or attribute name.
- `path`: relative or absolute attribute path to derivation.
- `version`: exact match on package `version` or parsed `name`.
- `semver`: semantic version range filter.
- `flake`: whether to search `packages` and `legacyPackages` outputs.
- `catalog`: whether to search `catalog` outputs.
- `stability`: catalog stability channel match - implies catalog.

### Example Descriptors
```json
{ "name": "hello" }
{ "path": ["legacyPackages", null, "python3Packages", "pip"] }
{ "name": "hello", "version": "2.10.1" }
{ "name": "nodejs", "semver": "^18.16" }
{ "name": "nodejs", "semver": ">=14 <18.16.0 || >18.16.0" }
{ "name": "hello", "input": "nixpkgs" }
{ "name": "hello", "catalog": true }
{ "name": "flox", "flake": false, "stability": "unstable" }
```


## Preferences

These settings control resolution behaviors, particularly priority ordering for
inputs and attribute-set prefixes to search under.

All of these fields are optional, and when left unset fallbacks will be used.

Remember that in addition to these preferences, the resolver carries a second
structure with information about inputs.
In our preferences we may refer to those inputs using their "alias"
or short-name.


## Example Settings

```json
{
  "inputs": ["nixpkgs", "nixpkgs-flox"],
  "allow": {
    "unfree":   false,
    "broken":   false,
    "licenses": ["MIT" "GPL3"]
  },
  "semver": {
    "preferPreReleases": true
  },
  "prefixes": {
    "nixpkgs":      ["legacyPackages", "packages", "catalog"],
    "nixpkgs-flox": ["catalog", "packages", "legacyPackages"]
  },
  "stabilities": {
    "nixpkgs-flox": ["unstable", "staging", "unstable"]
  }
}
```


## TODOs
- [ ] Allow preferences to limit systems list.
- [ ] Implement `preferPreReleases`.
- [ ] Read Inputs from `flake.lock` and `registry.json`.
- [ ] Parse descriptor strings.
- [ ] Additional tests.
- [ ] Multi-threading.
- [ ] Disable `builtins.trace` warnings.
- [ ] Consider how `packages.*.default` is handled.
- [ ] Handle stabilities in `DrvDb` progress table.
- [ ] Use `PackageSet` abstraction in `ResolverState::resolveInInput`.
- [ ] Executable to populate databases explicitly ( daemon? ).
- [ ] Sort results by version. This may require change to output format.
