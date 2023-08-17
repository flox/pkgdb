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

LIBFLOXPKGDB = libflox-pkgdb$(libExt)

LIBS           =  $(LIBFLOXPKGDB)
COMMON_HEADERS =  $(wildcard include/*.hh) $(wildcard include/flox/*.hh)
COMMON_HEADERS += $(wildcard include/flox/core/*.hh)
SRCS           =  $(wildcard src/*.cc) $(wildcard src/pkgdb/*.cc)
bin_SRCS       =  $(addprefix src/,main.cc scrape.cc get.cc pkgdb/command.cc)
lib_SRCS       =  $(filter-out $(bin_SRCS),$(SRCS))
test_SRCS      =  $(wildcard tests/*.cc)
BINS           =  pkgdb
TEST_UTILS     =  tests/is_sqlite3
TESTS          =  $(filter-out $(TEST_UTILS),$(test_SRCS:.cc=))


# ---------------------------------------------------------------------------- #

# You can disable these optional gripes with `make EXTRA_CXXFLAGS='' ...;'
EXTRA_CXXFLAGS ?= -Wall -Wextra -Wpedantic
CXXFLAGS       ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS       += '-I$(MAKEFILE_DIR)/include'
CXXFLAGS       += '-DFLOX_PKGDB_VERSION="$(VERSION)"'
LDFLAGS        ?= $(EXTRA_LDFLAGS)
lib_CXXFLAGS   ?= -shared -fPIC
ifeq (Linux,$(OS))
lib_LDFLAGS  ?= -shared -fPIC -Wl,--no-undefined
else
lib_LDFLAGS  ?= -shared -fPIC -Wl,-undefined,error
endif
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
CXXFLAGS += -DSEMVER_PATH='$(SEMVER_PATH)'


# ---------------------------------------------------------------------------- #

.PHONY: bin lib include tests

bin:     lib $(addprefix bin/,$(BINS))
lib:     $(addprefix lib/,$(LIBS))
include: $(COMMON_HEADERS)
tests:   $(TESTS) $(TEST_UTILS)


# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) $(addprefix bin/,$(BINS))
	-$(RM) $(addprefix lib/,$(LIBS))
	-$(RM) src/*.o tests/*.o
	-$(RM) result
	-$(RM) -r $(PREFIX)
	-$(RM) $(TESTS) $(TEST_UTILS)
	-$(RM) gmon.out *.log
	-$(MAKE) -C src/sql clean
	-$(RM) $(addprefix docs/,*.png *.html *.svg *.css *.js)
	-$(RM) -r docs/search


# ---------------------------------------------------------------------------- #

%.o: %.cc $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"


lib/$(LIBFLOXPKGDB): CXXFLAGS += $(lib_CXXFLAGS)
lib/$(LIBFLOXPKGDB): LDFLAGS  += $(lib_LDFLAGS)
lib/$(LIBFLOXPKGDB): $(lib_SRCS:.cc=.o)
	$(MKDIR_P) $(@D)
	$(CXX) $^ $(LDFLAGS) -o "$@"


# ---------------------------------------------------------------------------- #

bin/pkgdb: CXXFLAGS += $(bin_CXXFLAGS)
bin/pkgdb: LDFLAGS  += $(bin_LDFLAGS)
bin/pkgdb: $(bin_SRCS:.cc=.o) lib/$(LIBFLOXPKGDB)
	$(MKDIR_P) $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o "$@"


# ---------------------------------------------------------------------------- #

$(TESTS) $(TEST_UTILS): CXXFLAGS += $(bin_CXXFLAGS)
$(TESTS) $(TEST_UTILS): LDFLAGS  += $(bin_LDFLAGS)
$(TESTS) $(TEST_UTILS): tests/%: tests/%.cc lib/$(LIBFLOXPKGDB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$<" -o "$@"


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

.PHONY: ccls
ccls: .ccls

.ccls: FORCE
	echo 'clang' > "$@";
	{                                                                     \
	  if [[ -n "$(NIX_CC)" ]]; then                                       \
	    $(CAT) "$(NIX_CC)/nix-support/libc-cflags";                       \
	    $(CAT) "$(NIX_CC)/nix-support/libcxx-cxxflags";                   \
	  fi;                                                                 \
	  echo $(CXXFLAGS) $(sqlite3_CFLAGS) $(nljson_CFLAGS) $(nix_CFLAGS);  \
	  echo $(nljson_CFLAGS) $(argparse_CFLAGS) $(sql_builder_CFLAGS);     \
	  echo $(sqlite3pp_CFLAGS);                                           \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=/%cpp -std=/' >> "$@";


# ---------------------------------------------------------------------------- #

.PHONY: docs

docs: docs/index.html

docs/index.html: FORCE
	$(DOXYGEN) ./Doxyfile


# ---------------------------------------------------------------------------- #

# Generates SQL -> C++ schema headers

SQL_FILES    := $(wildcard src/sql/*.sql)
SQL_HH_FILES := $(SQL_FILES:.sql=.hh)

$(SQL_HH_FILES): %.hh: %.sql src/sql/Makefile
	$(MAKE) -C src/sql $(@F)

.PHONY: sql-headers
sql-headers: $(SQL_HH_FILES)

src/pkgdb.o: $(SQL_HH_FILES)


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
PC_CFLAGS += -DSEMVER_PATH='$(SEMVER_PATH)'
PC_LIBS   =  $(shell $(PKG_CONFIG) --libs-only-L nix-main) -lnixfetchers
lib/pkgconfig/flox-pkgdb.pc: lib/pkgconfig/flox-pkgdb.pc.in version
	$(SED) -e 's,@PREFIX@,$(PREFIX),g'     \
	       -e 's,@VERSION@,$(VERSION),g'   \
	       -e 's,@CFLAGS@,$(PC_CFLAGS),g'  \
	       -e 's,@LIBS@,$(PC_LIBS),g'      \
	       $< > $@

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
