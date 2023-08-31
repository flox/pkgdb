# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

MAKEFILE_DIR ?= $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ---------------------------------------------------------------------------- #

.PHONY: all clean FORCE ignores most
.DEFAULT_GOAL = most


# ---------------------------------------------------------------------------- #

CXX        ?= c++
RM         ?= rm -f
CAT        ?= cat
PKG_CONFIG ?= pkg-config
NIX        ?= nix
UNAME      ?= uname
MKDIR      ?= mkdir
MKDIR_P    ?= $(MKDIR) -p
CP         ?= cp
TR         ?= tr
SED        ?= sed
TEST       ?= test
DOXYGEN    ?= doxygen
BEAR       ?= bear
TIDY       ?= clang-tidy


# ---------------------------------------------------------------------------- #

OS ?= $(shell $(UNAME))
OS := $(OS)
ifndef libExt
ifeq (Linux,$(OS))
libExt ?= .so
else
libExt ?= .dylib
endif  # ifeq (Linux,$(OS))
endif  # ifndef libExt


# ---------------------------------------------------------------------------- #

VERSION := $(file < $(MAKEFILE_DIR)/version)


# ---------------------------------------------------------------------------- #

PREFIX     ?= $(MAKEFILE_DIR)/out
BINDIR     ?= $(PREFIX)/bin
LIBDIR     ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include


# ---------------------------------------------------------------------------- #

# rwildcard DIRS, PATTERNS
# ------------------------
# Recursive wildcard.
#   Ex:  $(call rwildcard,src,*.cc *.hh)
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2)        \
                                             $(filter $(subst *,%,$2),$d))


# ---------------------------------------------------------------------------- #

LIBFLOXPKGDB = libflox-pkgdb$(libExt)

LIBS           =  $(LIBFLOXPKGDB)
COMMON_HEADERS =  $(call rwildcard,include,*.hh)
SRCS           =  $(call rwildcard,src,*.cc)
bin_SRCS       =  $(addprefix src/,main.cc scrape.cc get.cc pkgdb/command.cc)
bin_SRCS       += $(addprefix src/,search/command.cc)
lib_SRCS       =  $(filter-out $(bin_SRCS),$(SRCS))
test_SRCS      =  $(wildcard tests/*.cc)
BINS           =  pkgdb
TEST_UTILS     =  $(addprefix tests/,is_sqlite3 parse-preferences)
TESTS          =  $(filter-out $(TEST_UTILS),$(test_SRCS:.cc=))
CLEANDIRS      =
CLEANFILES     =  $(SRCS:.cc=.o) $(test_SRCS:.cc=.o)
CLEANFILES     += $(addprefix bin/,$(BINS)) $(addprefix lib/,$(LIBS))
CLEANFILES     += $(TESTS) $(TEST_UTILS)


# ---------------------------------------------------------------------------- #

# You can disable these optional gripes with `make EXTRA_CXXFLAGS='' ...;'
EXTRA_CXXFLAGS ?= -Wall -Wextra -Wpedantic
CXXFLAGS       ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS       += '-I$(MAKEFILE_DIR)/include'
CXXFLAGS       += '-DFLOX_PKGDB_VERSION="$(VERSION)"'
LDFLAGS        ?= $(EXTRA_LDFLAGS)
lib_CXXFLAGS   ?= -fPIC
ifeq (Linux,$(OS))
lib_CXXFLAGS += -shared
endif
lib_LDFLAGS  ?= -shared -fPIC -Wl,-undefined,error
bin_CXXFLAGS ?=
bin_LDFLAGS  ?=

ifneq ($(DEBUG),)
CXXFLAGS += -ggdb3 -pg
LDFLAGS  += -ggdb3 -pg
endif


# ---------------------------------------------------------------------------- #

nljson_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags nlohmann_json)
nljson_CFLAGS := $(nljson_CFLAGS)

argparse_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags argparse)
argparse_CFLAGS := $(argparse_CFLAGS)

boost_CFLAGS    ?=                                                             \
  -I$(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#boost')/include
boost_CFLAGS := $(boost_CFLAGS)

sqlite3_CFLAGS  ?= $(shell $(PKG_CONFIG) --cflags sqlite3)
sqlite3_CFLAGS  := $(sqlite3_CFLAGS)
sqlite3_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs sqlite3)
sqlite3_LDLAGS  := $(sqlite3_LDLAGS)

sqlite3pp_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags sqlite3pp)
sqlite3pp_CFLAGS := $(sqlite3pp_CFLAGS)

nix_INCDIR  ?= $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_INCDIR  := $(nix_INCDIR)
ifndef nix_CFLAGS
nix_CFLAGS =  $(boost_CFLAGS)
nix_CFLAGS += $(shell $(PKG_CONFIG) --cflags nix-main nix-cmd nix-expr)
nix_CFLAGS += -isystem $(nix_INCDIR) -include $(nix_INCDIR)/nix/config.h
endif
nix_CFLAGS := $(nix_CFLAGS)

ifndef nix_LDFLAGS
nix_LDFLAGS =                                                        \
	$(shell $(PKG_CONFIG) --libs nix-main nix-cmd nix-expr nix-store)
nix_LDFLAGS += -lnixfetchers
endif
nix_LDFLAGS := $(nix_LDFLAGS)

ifndef flox_pkgdb_LDFLAGS
flox_pkgdb_LDFLAGS =  '-L$(MAKEFILE_DIR)/lib' -lflox-pkgdb
ifeq (Linux,$(OS))
flox_pkgdb_LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'
endif
endif


# ---------------------------------------------------------------------------- #

lib_CXXFLAGS += $(sqlite3_CFLAGS) $(sqlite3pp_CFLAGS)
bin_CXXFLAGS += $(argparse_CFLAGS)
CXXFLAGS     += $(nix_CFLAGS) $(nljson_CFLAGS)

ifeq (Linux,$(OS))
lib_LDFLAGS += -Wl,--as-needed
endif
lib_LDFLAGS += $(nix_LDFLAGS) $(sqlite3_LDFLAGS)
ifeq (Linux,$(OS))
lib_LDFLAGS += -Wl,--no-as-needed
endif

bin_LDFLAGS += $(nix_LDFLAGS) $(flox_pkgdb_LDFLAGS) $(sqlite3_LDFLAGS)


# ---------------------------------------------------------------------------- #

SEMVER_PATH ?=                                                        \
  $(shell $(NIX) build --no-link --print-out-paths                    \
	                     'github:aakropotkin/floco#semver')/bin/semver
CXXFLAGS += '-DSEMVER_PATH="$(SEMVER_PATH)"'


# ---------------------------------------------------------------------------- #

.PHONY: bin lib include tests

bin:     lib $(addprefix bin/,$(BINS))
lib:     $(addprefix lib/,$(LIBS))
include: $(COMMON_HEADERS)
tests:   $(TESTS) $(TEST_UTILS)


# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) $(CLEANFILES)
	-$(RM) -r $(CLEANDIRS)
	-$(RM) result
	-$(RM) gmon.out *.log


# ---------------------------------------------------------------------------- #

%.o: %.cc $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@


lib/$(LIBFLOXPKGDB): $(COMMON_HEADERS)
lib/$(LIBFLOXPKGDB): CXXFLAGS += $(lib_CXXFLAGS)
lib/$(LIBFLOXPKGDB): LDFLAGS  += $(lib_LDFLAGS)
lib/$(LIBFLOXPKGDB): $(lib_SRCS:.cc=.o)
	$(MKDIR_P) $(@D)
	$(CXX) $(filter %.o,$^) $(LDFLAGS) -o $@


# ---------------------------------------------------------------------------- #

src/pkgdb/write.o: src/pkgdb/schemas.hh

bin/pkgdb: $(COMMON_HEADERS)
bin/pkgdb: CXXFLAGS += $(bin_CXXFLAGS)
bin/pkgdb: LDFLAGS  += $(bin_LDFLAGS)
bin/pkgdb: $(bin_SRCS:.cc=.o) lib/$(LIBFLOXPKGDB)
	$(MKDIR_P) $(@D)
	$(CXX) $(CXXFLAGS) $(filter %.o,$^) $(LDFLAGS) -o $@


# ---------------------------------------------------------------------------- #

$(TESTS) $(TEST_UTILS): $(COMMON_HEADERS)
$(TESTS) $(TEST_UTILS): CXXFLAGS += $(bin_CXXFLAGS)
$(TESTS) $(TEST_UTILS): LDFLAGS  += $(bin_LDFLAGS)
$(TESTS) $(TEST_UTILS): tests/%: tests/%.cc lib/$(LIBFLOXPKGDB)
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS)  -o $@


# ---------------------------------------------------------------------------- #

.PHONY: install-dirs install-bin install-lib install-include install
install: install-dirs install-bin install-lib install-include

install-dirs: FORCE
	$(MKDIR_P) $(BINDIR) $(LIBDIR) $(LIBDIR)/pkgconfig
	$(MKDIR_P) $(INCLUDEDIR)/flox $(INCLUDEDIR)/flox/core $(INCLUDEDIR)/flox/pkgdb

$(INCLUDEDIR)/%: include/% | install-dirs
	$(CP) -- "$<" "$@"

$(LIBDIR)/%: lib/% | install-dirs
	$(CP) -- "$<" "$@"

$(BINDIR)/%: bin/% | install-dirs
	$(CP) -- "$<" "$@"

install-bin: $(addprefix $(BINDIR)/,$(BINS))
install-lib: $(addprefix $(LIBDIR)/,$(LIBS))
install-include:                                                    \
	$(addprefix $(INCLUDEDIR)/,$(subst include/,,$(COMMON_HEADERS)))


# ---------------------------------------------------------------------------- #

.PHONY: ccls cdb
ccls: .ccls
cdb: compile_commands.json

.ccls: FORCE
	echo 'clang' > "$@";
	{                                                                     \
	  if [[ -n "$(NIX_CC)" ]]; then                                       \
	    $(CAT) "$(NIX_CC)/nix-support/libc-cflags";                       \
	    $(CAT) "$(NIX_CC)/nix-support/libcxx-cxxflags";                   \
	  fi;                                                                 \
	  echo $(CXXFLAGS) $(sqlite3_CFLAGS) $(nljson_CFLAGS) $(nix_CFLAGS);  \
	  echo $(nljson_CFLAGS) $(argparse_CFLAGS) $(sqlite3pp_CFLAGS);       \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=/%cpp -std=/' >> "$@";


compile_commands.json: flake.nix flake.lock pkg-fun.nix
compile_commands.json: $(lastword $(MAKEFILE_LIST))
compile_commands.json: $(COMMON_HEADERS) $(SRCS) $(test_SRCS)
	-$(MAKE) -C $(MAKEFILE_DIR) clean;
	$(BEAR) --output "$@" -- $(MAKE) -C $(MAKEFILE_DIR) -j all;


# ---------------------------------------------------------------------------- #

.PHONY: lint
lint: compile_commands.json $(COMMON_HEADERS) $(SRCS) $(test_SRCS)
	$(TIDY) $(filter-out compile_commands.json,$^)


# ---------------------------------------------------------------------------- #

.PHONY: check cc-check bats-check

check: cc-check bats-check

cc-check: $(TESTS:.cc=)
	@_ec=0;                     \
	echo '';                    \
	for t in $(TESTS:.cc=); do  \
	  echo "Testing: $$t";      \
	  if "./$$t"; then          \
	    echo "PASS: $$t";       \
	  else                      \
	    _ec=1;                  \
	    echo "FAIL: $$t";       \
	  fi;                       \
	  echo '';                  \
	done;                       \
	exit "$$_ec"

bats-check: bin $(TEST_UTILS)
	PKGDB="$(MAKEFILE_DIR)/bin/pkgdb"                        \
	IS_SQLITE3="$(MAKEFILE_DIR)/tests/is_sqlite3"            \
	  bats --print-output-on-failure --verbose-run --timing  \
	       "$(MAKEFILE_DIR)/tests"


# ---------------------------------------------------------------------------- #

all: bin lib tests ignores
most: bin lib ignores


# ---------------------------------------------------------------------------- #

.PHONY: docs

docs: docs/index.html

docs/index.html: FORCE
	$(DOXYGEN) ./Doxyfile

CLEANFILES += $(addprefix docs/,*.png *.html *.svg *.css *.js)
CLEANDIRS  += docs/search


# ---------------------------------------------------------------------------- #

# Generate `pkg-config' file.
#
# The `PC_CFLAGS' and `PC_LIBS' variables carry flags that are not covered by
# `nlohmann_json`, `argparse`, `sqlite3pp`, `sqlite`, and `nix{main,cmd,expr}`
# `Requires' handling.
# This amounts to handling `boost', `libnixfetchers', forcing
# the inclusion of the `nix' `config.h' header, and some additional CPP vars.

PC_CFLAGS =  $(filter -std=%,$(CXXFLAGS))
PC_CFLAGS += -I$(nix_INCDIR) -include $(nix_INCDIR)/nix/config.h
PC_CFLAGS += $(boost_CFLAGS)
PC_CFLAGS += '-DFLOX_PKGDB_VERSION=\\\\\"$(VERSION)\\\\\"'
PC_CFLAGS += '-DSEMVER_PATH=\\\\\"$(SEMVER_PATH)\\\\\"'
PC_LIBS   =  $(shell $(PKG_CONFIG) --libs-only-L nix-main) -lnixfetchers
lib/pkgconfig/flox-pkgdb.pc: lib/pkgconfig/flox-pkgdb.pc.in version
	$(SED) -e 's,@PREFIX@,$(PREFIX),g'     \
	       -e 's,@VERSION@,$(VERSION),g'   \
	       -e 's,@CFLAGS@,$(PC_CFLAGS),g'  \
	       -e 's,@LIBS@,$(PC_LIBS),g'      \
	       $< > $@

CLEANFILES += lib/pkgconfig/flox-pkgdb.pc

install-lib: $(LIBDIR)/pkgconfig/flox-pkgdb.pc


# ---------------------------------------------------------------------------- #

ignores: tests/.gitignore
tests/.gitignore: FORCE
	$(MKDIR_P) $(@D)
	printf '%s\n' $(patsubst tests/%,%,$(test_SRCS:.cc=)) > $@


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
