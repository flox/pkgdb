# PkgDb Registry

A _registry_ in `pkgdb` is a structure used to organize a set of `nix` _flakes_,
called _inputs_ throughout this document, which have been assigned short-names
and additional metadata.

While the _registry_ structure in `pkgdb` is similar to that used by `nix`, it 
has been extended to record additional information for each _input_ related to
package resolution/search.

Additionally the registry carries a small number of _global_ settings such as
`priority` ( resolution/search ranking for each _input_ ), and
_default_/fallback settings to be used if/when _inputs_ do not explicitly
declare some settings.


## Schemas

Before diving into the details of individual parts of the schema, lets start
with an example of a registry with three _inputs_.
Here we use JSON, but any trivial format could be used.

```json
{
  "inputs": {
    "nixpkgs": {
      "from": {
        "type": "github"
      , "owner": "NixOS"
      , "repo": "nixpkgs"
      , "rev":  "e8039594435c68eb4f780f3e9bf3972a7399c4b1"
      }
    , "subtrees": ["legacyPackages"]
    }
  , "floco": {
      "from": {
        "type": "github"
      , "owner": "aakropotkin"
      , "repo": "floco"
      }
    , "subtrees": ["packages"]
    }
  , "nixpkgs-flox": {
      "from": {
        "type": "github"
      , "owner": "flox"
      , "repo": "nixpkgs-flox"
      , "rev": "feb593b6844a96dd4e17497edaabac009be05709"
      }
    , "subtrees": ["catalog"]
    , "stabilities": ["stable"]
    }
  , "floxpkgs": {
      "from": {
        "type": "github"
      , "owner": "flox"
      , "repo": "floxpkgs"
      }
    , "subtrees": ["catalog"]
    }
  }
, "defaults": {
    "subtrees": null
  , "stabilities": ["stable", "staging", "unstable"]
  }
, "priority": ["nixpkgs", "nixpkgs-flox"]
}
```

We'll dive into the details below that refer to this example.
