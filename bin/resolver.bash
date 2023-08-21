#! /usr/bin/env bash
# ============================================================================ #
#
# Rapid prototype `resolver' using `pkgdb' utility.
#
#
# ---------------------------------------------------------------------------- #

set -eu;
set -o pipefail;


# ---------------------------------------------------------------------------- #

_as_me="resolver";

_version="0.1.0";

_usage_msg="Usage: $_as_me [OPTIONS...] \
[--inputs INPUTS] [--preferences PREFERENCES] --descriptor DESCRIPTOR

Resolve nix package descriptors in flakes.
";

_help_msg="$_usage_msg

Optional arguments:
  -h, --help                    shows help message and exits
  -v, --version                 prints version information and exits
  -o, --one                     return single resolved entry or \`null'
  -q, --quiet                   exit 0 even if no resolutions are found
  -i, --inputs INPUTS           inline JSON or path to JSON file containing \
flake references
  -p, --preferences PREFERENCES inline JSON or path to JSON file containing \
resolver preferences [default: {}]
  -d, --descriptor DESCRIPTOR   inline JSON or path to JSON file containing a \
package descriptor [required]

Environment:
  SYSTEMS     Space separated list of systems to target ( default: current )

  CAT         Command used as \`cat' executable.
  GREP        Command used as \`grep' executable.
  REALPATH    Command used as \`realpath' executable.
  MKTEMP      Command used as \`mktemp' executable.
  SQLITE3     Command used as \`sqlite3' executable.
  PKGDB       Command used as \`pkgdb' executable.
  SEMVER      Command used as \`semver' executable.
  JQ          Command used as \`jq' executable.
  NIX         Command used as \`nix' executable.
";


# ---------------------------------------------------------------------------- #

usage() {
  if [[ "${1:-}" = "-f" ]]; then
    echo "$_help_msg";
  else
    echo "$_usage_msg";
  fi
}


# ---------------------------------------------------------------------------- #

# @BEGIN_INJECT_UTILS@
: "${CAT:=cat}";
: "${GREP:=grep}";
: "${REALPATH:=realpath}";
: "${MKTEMP:=mktemp}";
: "${SQLITE3:=sqlite3}";
: "${PKGDB:=pkgdb}";
: "${SEMVER:=semver}";
: "${JQ:=jq}";
: "${NIX:=nix}";


# ---------------------------------------------------------------------------- #

# If argument is a path to a JSON file dump it, otherwise assert that argument
# is valid JSON and print it's condensed form.
coerceJSON() { if [[ -r "$1" ]]; then $JQ -c "$1"; else echo "$1"|$JQ -c; fi }


# ---------------------------------------------------------------------------- #

_ONE=;
_QUIET=;
_INPUTS='{
  "nixpkgs":      "github:NixOS/nixpkgs"
, "nixpkgs-flox": "github:flox/nixpkgs-flox"
}';
_PREFS='{
  "prefixes": {
    "nixpkgs":      ["legacyPackages"]
  , "nixpkgs-flox": ["catalog"]
  }
, "stabilities": {
    "nixpkgs-flox": ["stable"]
  }
}';
unset _DESC;

while [[ "$#" -gt 0 ]]; do
  case "$1" in
    # Split short options such as `-abc' -> `-a -b -c'
    -[^-]?*)
      _arg="$1";
      declare -a _args;
      _args=();
      shift;
      _i=1;
      while [[ "$_i" -lt "${#_arg}" ]]; do
        _args+=( "-${_arg:$_i:1}" );
        _i="$(( _i + 1 ))";
      done
      set -- "${_args[@]}" "$@";
      unset _arg _args _i;
      continue;
    ;;
    --*=*)
      _arg="$1";
      shift;
      set -- "${_arg%%=*}" "${_arg#*=}" "$@";
      unset _arg;
      continue;
    ;;
    -q|--quiet)       _QUIET=:; ;;
    -o|--one)         _ONE=:; ;;
    -i|--inputs)      _INPUTS="$( coerceJSON "$2"; )"; shift; ;;
    -p|--preferences) _PREFS="$( coerceJSON "$2"; )"; shift; ;;
    -d|--descriptor)  _DESC="$( coerceJSON "$2"; )"; shift; ;;
    -u|--usage)       usage;    exit 0; ;;
    -h|--help)        usage -f; exit 0; ;;
    -v|--version)     echo "$_version"; exit 0; ;;
    --)               shift; break; ;;
    #-?|--*)
    #  echo "$_as_me: Unrecognized option: '$1'" >&2;
    #  usage -f >&2;
    #  exit 1;
    #;;
    *)
      echo "$_as_me: Unexpected argument(s) '$*'" >&2;
      usage -f >&2;
      exit 1;
    ;;
  esac
  shift;
done


# ---------------------------------------------------------------------------- #

if [[ -z "${_DESC}" ]]; then
  echo "$_as_me: You must provide a descriptor" >&2;
  usage >&2;
  exit 1;
fi


# ---------------------------------------------------------------------------- #

: "${NIX_SYSTEM=$( $NIX eval --raw --impure --expr builtins.currentSystem; )}";


# ---------------------------------------------------------------------------- #

# Systems to target
read -r -a systems <<< "${SYSTEMS:-$NIX_SYSTEM}";


# Load inputs
declare -A inputRefs inputRefsLocked inputDBs inputPrefixes inputStabilities;
mapfile -t inputs < <( echo "$_INPUTS"|$JQ -r 'keys[]'; );


# ---------------------------------------------------------------------------- #

# scrapeFlake INPUT-ID
# --------------------
scrapeFlake() {
  local _ref="${inputRefsLocked["$1"]}";
  read -r -a _subtrees    <<< "${inputPrefixes["$1"]}";
  read -r -a _stabilities <<< "${inputStabilities["$1"]}";
  if [[ -n "${VERBOSE:-}" ]]; then echo "Scraping input '$i'" >&2; fi
  for subtree in "${_subtrees[@]}"; do
    for system in "${systems[@]}"; do
      if [[ "$subtree" = 'catalog' ]]; then
        for stability in "${_stabilities[@]}"; do
          $PKGDB -q scrape "$_ref" "$subtree" "$system" "$stability" >/dev/null;
        done
      else
        $PKGDB -q scrape "$_ref" "$subtree" "$system" >/dev/null;
      fi
    done
  done
}


# ---------------------------------------------------------------------------- #

# show INPUT-ID PACKAGE-ID
# ------------------------
show() {
  local _jsonPre="{\"input\":\"$1\",\"path\":";
  echo "$_jsonPre$( $PKGDB get path --pkg "${inputDBs["$1"]}" "$2"; )}";
}


# ---------------------------------------------------------------------------- #

# Scrape Flakes

for i in "${inputs[@]}"; do
  inputRefs["$i"]="$( echo "$_INPUTS"|$JQ -r ".[\"$i\"]"; )";
  inputRefsLocked["$i"]="$(
    $NIX flake metadata --json "${inputRefs["$i"]}"|$JQ -r .url;
  )";
  inputDBs["$i"]="$( $PKGDB get db "${inputRefsLocked["$i"]}"; )";

  # Determing which prefixes ( subtrees ) to search under.
  inputPrefixes["$i"]="$(
    echo "$_PREFS"|$JQ -r "( .prefixes[\"$i\"] // [] )|join( \" \" )";
  )";
  if [[ -z "${inputPrefixes["$i"]}" ]]; then
    if $NIX eval --expr "let
         flake = builtins.getFlake \"${inputRefsLocked["$i"]}\";
       in assert ( flake ? catalog ); true
       " >/dev/null 2>&1;
    then
      inputPrefixes["$i"]=catalog;
    else
      inputPrefixes["$i"]='packages legacyPackages';
    fi
  fi

  # Determine which stabilities to search under.
  inputStabilities["$i"]="$(
    echo "$_PREFS"  \
      |$JQ -r "( .stabilities[\"$i\"] // [\"stable\"] )|join( \" \" )";
    )"
  scrapeFlake "$i";
done


# ---------------------------------------------------------------------------- #

_QUERY='SELECT id FROM PACKAGES WHERE TRUE';

if echo "$_PREFS"|$JQ -e '( .allow.unfree // true )|not' >/dev/null; then
  _QUERY+=' AND ( ( unfree IS NULL ) OR ( NOT unfree ) )';
fi

if echo "$_PREFS"|$JQ -e '( .allow.broken // false )|not' >/dev/null; then
  _QUERY+=' AND ( ( broken IS NULL ) OR ( NOT broken ) )';
fi

if echo "$_PREFS"|$JQ -e '( .allow.licenses // null ) != null' >/dev/null; then
  _QUERY+="AND ( ( license IS NULL ) OR ";
  _QUERY+="( license IN $( $JQ -r '.allow.licenses|join( " " )' ) ) )";
fi

_qname="$( echo "$_DESC"|$JQ -r '.name // ""'; )";
if [[ -n "$_qname" ]]; then
  _QUERY+=" AND ( ( pname = '$_qname' ) OR ( name = '$_qname' ) )";
fi

_qversion="$( echo "$_DESC"|$JQ -r '.version // ""'; )";
if [[ -n "$_qversion" ]]; then
  _QUERY+=" AND ( version = '$_qversion' )";
fi

# TODO: Sort by version
# TODO: Merge over system


# ---------------------------------------------------------------------------- #

if [[ -n "${VERBOSE:-}" ]]; then echo "$_QUERY" >&2; fi

# TODO: path, stability, catalog, flake

# _qsemver="$( echo "$_DESC"|$JQ -r '.semver // ""'; )";
# if [[ -z "$_qsemver" ]]; then
#   # TODO
# fi

# runQuery
# --------
# Query SQL databases and print satisfactory packages as JSON.
# If `--one' was given, print a single JSON object, otherwise print an array.
runQuery() {
  # If an input was explicitly indicated we only have to search one database.
  _qinput="$( echo "$_DESC"|$JQ -r '.input // ""'; )";
  if [[ -n "$_qinput" ]]; then
    mapfile -t matches < <( $SQLITE3 "${inputDBs["$_qinput"]}" "$_QUERY"; );
    if [[ "${#matches[@]}" -lt 1 ]]; then
      echo "$_as_me: no satisfactory results" >&2;
      exit 1;
    else
      show "$_qinput" "${matches[0]}";
      exit 0;
    fi
  fi

  first=:;
  for i in "${inputs[@]}"; do
    mapfile -t matches < <( $SQLITE3 "${inputDBs["$i"]}" "$_QUERY"; );
    if [[ "${#matches[@]}" -gt 0 ]]; then
      if [[ -n "$_ONE" ]]; then
        show "$i" "${matches[0]}";
        exit 0;
      else
        for p in "${matches[@]}"; do
          if [[ -z "$first" ]]; then
            printf ', ';
          else
            printf '[ ';
            first=;
          fi
          show "$i" "$p";
        done
      fi
    fi
  done

  if [[ -z "$first" ]]; then
    echo "]";
    return 0;
  else
    echo "$_as_me: no satisfactory results" >&2;
    return 1;
  fi
}


# ---------------------------------------------------------------------------- #

runQuery;
exit;


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
