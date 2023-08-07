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

PREFIX     ?= $(MAKEFILE_DIR)/out
BINDIR     ?= $(PREFIX)/bin
LIBDIR     ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include


# ---------------------------------------------------------------------------- #

LIBFLOXPKGDB = libflox-pkgdb$(libExt)

LIBS           = $(LIBFLOXPKGDB)
COMMON_HEADERS = $(wildcard include/*.hh) $(wildcard include/flox/*.hh)
SRCS           = $(wildcard src/*.cc)
bin_SRCS       = src/main.cc
lib_SRCS       = $(filter-out $(bin_SRCS),$(SRCS))
test_SRCS      = $(wildcard tests/*.cc)
BINS           = pkgdb
TESTS          = $(test_SRCS:.cc=)


# ---------------------------------------------------------------------------- #

CXXFLAGS     ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS     += '-I$(MAKEFILE_DIR)/include'
LDFLAGS      ?= $(EXTRA_LDFLAGS)
lib_CXXFLAGS ?= -shared -fPIC
lib_LDFLAGS  ?= -shared -fPIC -Wl,--no-undefined
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

sql_builder_CFLAGS ?=                                      \
  -I$(shell $(NIX) build --no-link --print-out-paths       \
                   '$(MAKEFILE_DIR)#sql-builder')/include
sql_builder_CFLAGS := $(sql_builder_CFLAGS)

sqlite3pp_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags sqlite3pp)
sqlite3pp_CFLAGS := $(sqlite3pp_CFLAGS)

nix_INCDIR  ?= $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_INCDIR  := $(nix_INCDIR)
ifndef nix_CFLAGS
  nix_CFLAGS  =  $(boost_CFLAGS)
  nix_CFLAGS  += $(shell $(PKG_CONFIG) --cflags nix-main nix-cmd nix-expr)
  nix_CFLAGS  += -isystem $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
  nix_CFLAGS  += -include $(nix_INCDIR)/nix/config.h
endif
nix_CFLAGS := $(nix_CFLAGS)

ifndef nix_LDFLAGS
  nix_LDFLAGS =                                                        \
	  $(shell $(PKG_CONFIG) --libs nix-main nix-cmd nix-expr nix-store)
  nix_LDFLAGS += -lnixfetchers
endif
nix_LDFLAGS := $(nix_LDFLAGS)

ifndef floxresolve_LDFLAGS
	floxresolve_LDFLAGS =  '-L$(MAKEFILE_DIR)/lib' -lflox-pkgdb
	floxresolve_LDFLAGS += -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'
endif


# ---------------------------------------------------------------------------- #

lib_CXXFLAGS += $(sqlite3_CFLAGS) $(sql_builder_CFLAGS) $(sqlite3pp_CFLAGS)
bin_CXXFLAGS += $(argparse_CFLAGS)
CXXFLAGS     += $(nix_CFLAGS) $(nljson_CFLAGS)

lib_LDFLAGS += -Wl,--as-needed
lib_LDFLAGS += $(nix_LDFLAGS) $(sqlite3_LDFLAGS)
lib_LDFLAGS += -Wl,--no-as-needed

bin_LDFLAGS += $(nix_LDFLAGS) $(floxresolve_LDFLAGS) $(sqlite3_LDFLAGS)


# ---------------------------------------------------------------------------- #

SEMVER_PATH ?=                                                        \
  $(shell $(NIX) build --no-link --print-out-paths                    \
	                     'github:aakropotkin/floco#semver')/bin/semver
CXXFLAGS += -DSEMVER_PATH='$(SEMVER_PATH)'


# ---------------------------------------------------------------------------- #

.PHONY: bin lib include tests

bin:     $(addprefix bin/,$(BINS))
lib:     $(addprefix lib/,$(LIBS))
include: $(COMMON_HEADERS)
tests:   $(TESTS)


# ---------------------------------------------------------------------------- #

clean: FORCE
	-$(RM) $(addprefix bin/,$(BINS))
	-$(RM) $(addprefix lib/,$(LIBS))
	-$(RM) src/*.o tests/*.o
	-$(RM) result
	-$(RM) -r $(PREFIX)
	-$(RM) $(TESTS)
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
bin/pkgdb: src/main.o lib/$(LIBFLOXPKGDB)
	$(MKDIR_P) $(@D)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$<" -o "$@"


# ---------------------------------------------------------------------------- #

$(TESTS): CXXFLAGS += $(bin_CXXFLAGS)
$(TESTS): LDFLAGS  += $(bin_LDFLAGS)
$(TESTS): tests/%: tests/%.cc lib/$(LIBFLOXPKGDB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) "$<" -o "$@"


# ---------------------------------------------------------------------------- #

.PHONY: install-dirs install-bin install-lib install-include install
install: install-dirs install-bin install-lib install-include

install-dirs: FORCE
	$(MKDIR_P) $(BINDIR) $(LIBDIR) $(INCLUDEDIR)/flox

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

bats-check: bin
	PKGDB="$(MAKEFILE_DIR)/bin/pkgdb"                        \
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

ignores: tests/.gitignore
tests/.gitignore: FORCE
	printf '%s\n' $(patsubst tests/%,%,$(TESTS)) > $@


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
