# Flox Package Database

CRUD operations on `nix` package metadata.

[Documentation](https://flox.github.io/pkgdb/index.html)

### Purpose

Evaluating nix expressions for an entire flake is expensive but necessary for features like package search. This tool provides a way to scrape the data from a flake once and store it in a database for later usage.

The current responsibility of the `pkgdb` tool extends only as far as scraping a flake and generating a database. The database should be queried using standard sqlite tools and libraries and all decisions about how and when to generate and update the database are left up to the consumer.

### Compilation

```bash
$ nix develop;
$ make -j8;  # set `-j<# Jobs>' as desired
```

### Usage

Build the database with the `scrape` subcommand:

```bash
$ pkgdb scrape github:NixOS/nixpkgs;
fetching flake 'github:NixOS/nixpkgs'...
/Users/me/.cache/flox/pkgdb-v0/93a89abd052c90a33e8787a7740f2459cdb496980848011ae708b0de1bbfac82.sqlite
```

By default, packages will be scraped from packages.[system arch] and stored in `~/.cache` in a database named after the flake fingerprint. These can be overridden as desired:

```bash
$ pkgdb scrape github:NixOS/nixpkgs --database flakedb.sqlite legacyPackages aarch64-darwin
```

If the database for a given flake already exists and is asked to reprocess an existing package set, it will be skipped. Use `--force` to force an update/regeneration.

Once generated, the database can be opened and queried using `sqlite3`.

```bash
$ sqlite3 flakedb.sqlite '.mode json' 'SELECT name, version FROM Packages LIMIT 10';
[{"name":"AMB-plugins-0.8.1","version":"0.8.1"},
{"name":"ArchiSteamFarm-5.4.7.3","version":"5.4.7.3"},
{"name":"AusweisApp2-1.26.7","version":"1.26.7"},
{"name":"BeatSaberModManager-0.0.5","version":"0.0.5"},
{"name":"CHOWTapeModel-2.10.0","version":"2.10.0"},
{"name":"CertDump-unstable-2023-07-12","version":"unstable-2023-07-12"},
{"name":"ChowCentaur-1.4.0","version":"1.4.0"},
{"name":"ChowKick-1.1.1","version":"1.1.1"},
{"name":"ChowPhaser-1.1.1","version":"1.1.1"},
{"name":"CoinMP-1.8.4","version":"1.8.4"}]
```

#### Expected Client Usage
This utility is expected to be run multiple times if a client wishes to "fully scrape all the things" in a flake.
This utility is a plumbing command used by a client application, we aren't particuarly concerned with the repetitive
strain injury a user would suffer if they tried to scrape everything in a flake interactively; rather we aim to do
less in a single run and avoid scraping info the caller might not need for their use case.

A given client application that did want to scrape a flake completely would run something along the lines of:
```shell
$ lockedRef="github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";
$ dbPath=;
$ for subtree in packages legacyPackages catalog; do
    for system in x86_64-linux x86_64-darwin aarch64-darwin aarch64-linux; do
      if [[ "$subtree" = 'catalog' ]]; then
        for stability in unstable staging stable; do
          if [[ -z "$dbPath" ]]; then  # get the DB name
            dbPath="$( pkgdb scrape "$lockedRef" "$subtree" "$system" "$stability"; )";
          else
            pkgdb scrape "$lockedRef" "$subtree" "$system" "$stability";
          fi
        done
      else
        pkgdb scrape "$lockedRef" "$subtree" "$system";
      fi
    done
  done
$ sqlite3 "$dbPath" 'SELECT COUNT( * ) FROM Packages';
```
In the example above we the caller would passes in a locked ref, this was technically optional, but is strongly recommended.
What's important is that invocations that intend to append to an existing database ABSOLUTELY SHOULD be using
locked flake references.
In the event that you want to use an unlocked reference on the first call, you can extract a locked flake reference from a
database for later runs, but official recommendation is to lock flakes before looping.

If the caller _really_ wants to they could pass an unlocked ref on the first invocation, and yank the locked reference from
the resulting database.
This is potentially useful for working with local flakes in the event that you don't want to use a utility like
`nix flake prefetch` or `parser-util` to lock your references for you :
```shell
$ dbPath="$( pkgdb scrape "$PWD/my-flake" packages x86_64-linux; )";
$ lockedRef="$( sqlite3 "$dbPath" 'SELECT string FROM LockedFlake'; )";
$ pkgdb scrape "$lockedRef" packages aarch64-linux;
...<SNIP>...
```

### Schema

The data is represented in a tree format matching the attrPath structure.
The two entities are AttrSets (branches) and Packages (leaves). Packages and AttrSets each have a parentId, which is always found in AttrSets.

Descriptions are de-duplicated (for instance between two packages for separate architectures) by a Descriptions table.

`DbVersions` and `LockedFlake` tables store metadata about the version of `pkgdb` that generated the database and the flake which was scraped.

#### Details

If they are defined explicitly, `pname` and `version` will be read from the
corresponding attributes.
Otherwise, they will be parsed from the `name`.
If `version` can be converted to a semver, it will be.

Note that the `attrName` for a package is the actual name in the tree.
Therefore, for catalogs the `attrName` will be the package version, not
the name.

If `outputsToInstall` is not defined, it will be the set of `outputs` up to and
including `"out"`.

```mermaid
erDiagram
  AttrSets ||--o{ Packages : "contains"
  AttrSets ||--o{ AttrSets : "contains nested"
  Packages ||--|| Descriptions : "described by"
  Packages {
    int id
    int parentId
    text attrName
    text name
    text pname
    text version
    text semver
    text license
    json outputs
    json outputsToInstall
    bool broken
    bool unfree
    int descriptionId
  }
  AttrSets {
    int id
    int parent
    text attrName
    bool done
  }
  DbVersions {
    text name
    text version
  }
  Descriptions {
    int id
    text description
  }
  LockedFlake {
    text fingerprint
    text string
    json attrs
  }
```
