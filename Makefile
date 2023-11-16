# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

MAKEFILE_DIR ?= $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ---------------------------------------------------------------------------- #

.PHONY: all clean fullclean FORCE ignores most
.DEFAULT_GOAL = most


# ---------------------------------------------------------------------------- #

BEAR       ?= bear
CAT        ?= cat
CP         ?= cp
CXX        ?= c++
DOXYGEN    ?= doxygen
FMT        ?= clang-format
GREP       ?= grep
LN         ?= ln -f
MKDIR      ?= mkdir
MKDIR_P    ?= $(MKDIR) -p
NIX        ?= nix
PKG_CONFIG ?= pkg-config
RM         ?= rm -f
SED        ?= sed
TEE        ?= tee
TEST       ?= test
TIDY       ?= clang-tidy
TOUCH      ?= touch
TR         ?= tr
UNAME      ?= uname


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
bin_SRCS       =  src/main.cc
bin_SRCS       += $(addprefix src/pkgdb/,scrape.cc get.cc command.cc)
bin_SRCS       += $(addprefix src/search/,command.cc)
bin_SRCS       += $(addprefix src/resolver/,command.cc)
lib_SRCS       =  $(filter-out $(bin_SRCS),$(SRCS))
test_SRCS      =  $(sort $(wildcard tests/*.cc))
ALL_SRCS       = $(SRCS) $(test_SRCS)
BINS           =  pkgdb
TEST_UTILS     =  $(addprefix tests/,is_sqlite3 search-params)
TESTS          =  $(filter-out $(TEST_UTILS),$(test_SRCS:.cc=))
CLEANDIRS      =
CLEANFILES     =  $(ALL_SRCS:.cc=.o)
CLEANFILES     += $(addprefix bin/,$(BINS)) $(addprefix lib/,$(LIBS))
CLEANFILES     += $(TESTS) $(TEST_UTILS)
FULLCLEANDIRS  =
FULLCLEANFILES =

TEST_DATA_DIR = $(MAKEFILE_DIR)/tests/data


# ---------------------------------------------------------------------------- #

# Files which effect dependencies, external inputs, and `*FLAGS' values.
DEPFILES =  flake.nix flake.lock pkg-fun.nix pkgs/nlohmann_json.nix
DEPFILES += pkgs/nix/pkg-fun.nix


# ---------------------------------------------------------------------------- #

# You can disable these optional gripes with `make EXTRA_CXXFLAGS='' ...;'
ifndef EXTRA_CXXFLAGS
EXTRA_CXXFLAGS = -Wall -Wextra -Wpedantic
ifneq (Linux,$(OS))
# Clang
EXTRA_CXXFLAGS += -Wno-gnu-zero-variadic-macro-arguments
endif  # ifneq (Linux,$(OS))
endif	# ifndef EXTRA_CXXFLAGS

CXXFLAGS       ?= $(EXTRA_CFLAGS) $(EXTRA_CXXFLAGS)
CXXFLAGS       += '-I$(MAKEFILE_DIR)/include'
CXXFLAGS       += '-DFLOX_PKGDB_VERSION="$(VERSION)"'
LDFLAGS        ?= $(EXTRA_LDFLAGS)

ifeq (Linux,$(OS))
# GCC
lib_CXXFLAGS ?= -shared -fPIC
lib_LDFLAGS  ?= -shared -fPIC -Wl,--no-undefined
else
# Clang
lib_CXXFLAGS   ?= -fPIC
lib_LDFLAGS    ?= -shared -fPIC -Wl,-undefined,error
endif # ifeq (Linux,$(OS))

bin_CXXFLAGS ?=
bin_LDFLAGS  ?=

# Debug Mode
ifneq ($(DEBUG),)
ifeq (Linux,$(OS))
# GCC
CXXFLAGS += -ggdb3 -pg
LDFLAGS  += -ggdb3 -pg
else
# Clang
CXXFLAGS += -g -pg
LDFLAGS  += -g -pg
endif # ifeq (Linux,$(OS))
endif # ifneq ($(DEBUG),)

# Coverage Mode
ifneq ($(COV),)
CXXFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS  += -fprofile-arcs -ftest-coverage
endif # ifneq ($(COV),)


# ---------------------------------------------------------------------------- #

nljson_CFLAGS ?=                                                            \
	$(patsubst -I%,-isystem %,$(shell $(PKG_CONFIG) --cflags nlohmann_json))
nljson_CFLAGS := $(nljson_CFLAGS)

argparse_CFLAGS ?=                                                     \
	$(patsubst -I%,-isystem %,$(shell $(PKG_CONFIG) --cflags argparse))
argparse_CFLAGS := $(argparse_CFLAGS)

boost_CFLAGS ?=                                                              \
  -isystem                                                                   \
  $(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#boost')/include
boost_CFLAGS := $(boost_CFLAGS)

toml_CFLAGS ?=                                                                \
  -isystem                                                                    \
  $(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#toml11')/include
toml_CFLAGS := $(toml_CFLAGS)

sqlite3_CFLAGS ?=                                                     \
	$(patsubst -I%,-isystem %,$(shell $(PKG_CONFIG) --cflags sqlite3))
sqlite3_CFLAGS  := $(sqlite3_CFLAGS)
sqlite3_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs sqlite3)
sqlite3_LDLAGS  := $(sqlite3_LDLAGS)

sqlite3pp_CFLAGS ?=                                                     \
	$(patsubst -I%,-isystem %,$(shell $(PKG_CONFIG) --cflags sqlite3pp))
sqlite3pp_CFLAGS := $(sqlite3pp_CFLAGS)

yaml_PREFIX ?=                                                          \
	$(shell $(NIX) build --no-link --print-out-paths 'nixpkgs#yaml-cpp')
yaml_PREFIX := $(yaml_PREFIX)
yaml_CFLAGS  = -isystem $(yaml_PREFIX)/include
yaml_LDFLAGS = -L$(yaml_PREFIX)/lib -lyaml-cpp

nix_INCDIR ?= $(shell $(PKG_CONFIG) --variable=includedir nix-cmd)
nix_INCDIR := $(nix_INCDIR)
ifndef nix_CFLAGS
_nix_PC_CFLAGS =  $(shell $(PKG_CONFIG) --cflags nix-main nix-cmd nix-expr)
nix_CFLAGS     =  $(boost_CFLAGS) $(patsubst -I%,-isystem %,$(_nix_PC_CFLAGS))
nix_CFLAGS     += -include $(nix_INCDIR)/nix/config.h
endif # ifndef nix_CFLAGS
nix_CFLAGS := $(nix_CFLAGS)
undefine _nix_PC_CFLAGS

ifndef nix_LDFLAGS
nix_LDFLAGS =                                                        \
	$(shell $(PKG_CONFIG) --libs nix-main nix-cmd nix-expr nix-store)
nix_LDFLAGS += -lnixfetchers
endif # ifndef nix_LDFLAGS
nix_LDFLAGS := $(nix_LDFLAGS)

ifndef flox_pkgdb_LDFLAGS
ifeq (Linux,$(OS))
flox_pkgdb_LDFLAGS = -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN/../lib'
else  # Darwin
ifneq "$(findstring install,$(MAKECMDGOALS))" ""
flox_pkgdb_LDFLAGS = '-L$(LIBDIR)'
endif # ifneq $(,$(findstring install,$(MAKECMDGOALS)))
endif # ifeq (Linux,$(OS))
flox_pkgdb_LDFLAGS += '-L$(MAKEFILE_DIR)/lib' -lflox-pkgdb
endif # ifndef flox_pkgdb_LDFLAGS


# ---------------------------------------------------------------------------- #

lib_CXXFLAGS += $(sqlite3pp_CFLAGS)
bin_CXXFLAGS += $(argparse_CFLAGS)
CXXFLAGS     += $(nix_CFLAGS) $(nljson_CFLAGS) $(toml_CFLAGS) $(yaml_CFLAGS)

ifeq (Linux,$(OS))
lib_LDFLAGS += -Wl,--as-needed
endif # ifeq (Linux,$(OS))

lib_LDFLAGS += $(nix_LDFLAGS) $(sqlite3_LDFLAGS)

ifeq (Linux,$(OS))
lib_LDFLAGS += -Wl,--no-as-needed
endif # ifeq (Linux,$(OS))

bin_LDFLAGS += $(nix_LDFLAGS) $(flox_pkgdb_LDFLAGS) $(sqlite3_LDFLAGS)
LDFLAGS     += $(yaml_LDFLAGS)


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
	-$(RM) $(CLEANFILES);
	-$(RM) -r $(CLEANDIRS);
	-$(RM) result;
	-$(RM) **/gmon.out gmon.out **/*.log *.log;
	-$(RM) **/*.gcno *.gcno **/*.gcda *.gcda **/*.gcov *.gcov;


fullclean: clean
	-$(RM) $(FULLCLEANFILES);
	-$(RM) -r $(FULLCLEANDIRS);


# ---------------------------------------------------------------------------- #

%.o: %.cc $(COMMON_HEADERS)
	@$(CXX) $(CXXFLAGS) -c $< -o $@;

ifeq (Linux,$(OS))
SONAME_FLAG = -Wl,-soname,$(LIBFLOXPKGDB)
else
SONAME_FLAG =
endif

lib/$(LIBFLOXPKGDB): CXXFLAGS += $(lib_CXXFLAGS)
lib/$(LIBFLOXPKGDB): $(lib_SRCS:.cc=.o)
	@$(MKDIR_P) $(@D);
	@$(CXX) $(filter %.o,$^) $(LDFLAGS) $(lib_LDFLAGS) $(SONAME_FLAG) -o $@;


# ---------------------------------------------------------------------------- #

src/pkgdb/write.o: src/pkgdb/schemas.hh

$(bin_SRCS:.cc=.o): %.o: %.cc $(COMMON_HEADERS)
	@$(CXX) $(CXXFLAGS) $(bin_CXXFLAGS) -c $< -o $@;

bin/pkgdb: $(bin_SRCS:.cc=.o) lib/$(LIBFLOXPKGDB)
	@$(MKDIR_P) $(@D);
	@$(CXX) $(filter %.o,$^) $(LDFLAGS) $(bin_LDFLAGS) -o $@;


# ---------------------------------------------------------------------------- #

$(TESTS) $(TEST_UTILS): $(COMMON_HEADERS)
$(TESTS) $(TEST_UTILS): bin_CXXFLAGS += '-DTEST_DATA_DIR="$(TEST_DATA_DIR)"'
$(TESTS) $(TEST_UTILS): tests/%: tests/%.cc lib/$(LIBFLOXPKGDB)
	@$(CXX) $(CXXFLAGS) $(bin_CXXFLAGS) $< $(LDFLAGS) $(bin_LDFLAGS) -o $@;


# ---------------------------------------------------------------------------- #

.PHONY: install-dirs install-bin install-lib install-include install
install: install-dirs install-bin install-lib install-include

install-dirs: FORCE
	@$(MKDIR_P) $(BINDIR) $(LIBDIR) $(LIBDIR)/pkgconfig;
	@$(MKDIR_P) $(INCLUDEDIR)/flox $(INCLUDEDIR)/flox/core;
	@$(MKDIR_P) $(INCLUDEDIR)/flox/pkgdb $(INCLUDEDIR)/flox/search;
	@$(MKDIR_P) $(INCLUDEDIR)/flox/resolver $(INCLUDEDIR)/compat;

$(INCLUDEDIR)/%: include/% | install-dirs
	@$(CP) -- "$<" "$@";

$(LIBDIR)/%: lib/% | install-dirs
	@$(CP) -- "$<" "$@";

$(BINDIR)/%: bin/% | install-dirs
	@$(CP) -- "$<" "$@";

# Darwin has to relink
ifneq (Linux,$(OS))
LINK_INAME_FLAG = -Wl,-install_name,$(LIBDIR)/$(LIBFLOXPKGDB)
$(LIBDIR)/$(LIBFLOXPKGDB): CXXFLAGS += $(lib_CXXFLAGS)
$(LIBDIR)/$(LIBFLOXPKGDB): $(lib_SRCS:.cc=.o)
	@$(MKDIR_P) $(@D);
	@$(CXX) $(filter %.o,$^) $(LDFLAGS) $(lib_LDFLAGS) $(LINK_INAME_FLAG)  \
	       -o $@;

$(BINDIR)/pkgdb: $(bin_SRCS:.cc=.o) $(LIBDIR)/$(LIBFLOXPKGDB)
	@$(MKDIR_P) $(@D);
	@$(CXX) $(filter %.o,$^) $(LDFLAGS) $(bin_LDFLAGS) -o $@;
endif # ifneq (Linux,$(OS))

install-bin: $(addprefix $(BINDIR)/,$(BINS))
install-lib: $(addprefix $(LIBDIR)/,$(LIBS))
install-include:                                                     \
	$(addprefix $(INCLUDEDIR)/,$(subst include/,,$(COMMON_HEADERS)));


# ---------------------------------------------------------------------------- #

# The nix builder deletes many of these files and they aren't used inside of
# the nix build environment.
# We need to ensure that these files exist nonetheless to satisfy prerequisites.
$(DEPFILES): %:
	if ! $(TEST) -e $<; then $(TOUCH) $@; fi


# ---------------------------------------------------------------------------- #

# Create pre-compiled-headers specifically so that we can force our headers
# to appear in `compile_commands.json'.
# We don't actually use these in our build.
.PHONY: pre-compiled-headers clean-pch

PRE_COMPILED_HEADERS = $(patsubst %,%.gch,$(COMMON_HEADERS))
CLEANFILES += $(PRE_COMPILED_HEADERS)

$(PRE_COMPILED_HEADERS): CXXFLAGS += $(lib_CXXFLAGS) $(bin_CXXFLAGS)
$(PRE_COMPILED_HEADERS): $(COMMON_HEADERS) $(DEPFILES)
$(PRE_COMPILED_HEADERS): $(lastword $(MAKEFILE_LIST))
$(PRE_COMPILED_HEADERS): %.gch: %
	@echo "Generating pre-compiled header \`$@' ( warnings are suppressed )" >&2;
	@$(CXX) $(CXXFLAGS) -x c++-header -c $< -o $@ 2>/dev/null;

pre-compiled-headers: $(PRE_COMPILED_HEADERS)

# Remove all `pre-compiled-headers'.
# This is used when creating our compilation databases to ensure that
# pre-compiled headers aren't taking priority over _real_ headers.
clean-pch: FORCE
	@$(RM) $(PRE_COMPILED_HEADERS);


# ---------------------------------------------------------------------------- #

# Create `.ccls' file used by CCLS LSP as a fallback when a file is undefined
# in `compile_commands.json'.
# This will be ignored by other LSPs such as `clangd'.

.PHONY: ccls
ccls: .ccls

.ccls: $(lastword $(MAKEFILE_LIST)) $(DEPFILES)
	@echo '%compile_commands.json' > "$@";
	{                                                                     \
	  if $(TEST) -n "$(NIX_CC)"; then                                     \
	    $(CAT) "$(NIX_CC)/nix-support/libc-cflags";                       \
	    $(CAT) "$(NIX_CC)/nix-support/libcxx-cxxflags";                   \
	  fi;                                                                 \
	  echo $(CXXFLAGS) $(nljson_CFLAGS) $(nix_CFLAGS);                    \
	  echo $(argparse_CFLAGS) $(sqlite3pp_CFLAGS);                        \
	  echo '-DTEST_DATA_DIR="$(TEST_DATA_DIR)"';                          \
	}|$(TR) ' ' '\n'|$(SED) 's/-std=\(.*\)/%cpp -std=\1|%h -std=\1/'      \
	 |$(TR) '|' '\n' >> "$@";

FULLCLEANFILES += .ccls


# ---------------------------------------------------------------------------- #

# Create `compile_commands.json' file used by LSPs.

# Get system include paths from `nix' C++ compiler.
# Filter out framework directory, e.g.
# /nix/store/q2d0ya7rc5kmwbwvsqc2djvv88izn1q6-apple-framework-CoreFoundation-11.0.0/Library/Frameworks (framework directory)
# We might be able to strip '(framework directory)' instead and append
# CoreFoundation.framework/Headers but I don't think we need to.
_CXX_SYSTEM_INCDIRS := $(shell                                \
  $(CXX) -E -Wp,-v -xc++ /dev/null 2>&1 1>/dev/null           \
  |$(GREP) -v 'framework directory'|$(GREP) '^ /nix/store')
_CXX_SYSTEM_INCDIRS := $(patsubst %,-isystem %,$(_CXX_SYSTEM_INCDIRS))

BEAR_WRAPPER := $(dir $(shell command -v $(BEAR)))
BEAR_WRAPPER := $(dir $(patsubst %/,%,$(BEAR_WRAPPER)))lib/bear/wrapper

bear.d/c++:
	@$(MKDIR_P) $(@D);
	@$(LN) -s $(BEAR_WRAPPER) bear.d/c++;

FULLCLEANDIRS += bear.d

compile_commands.json: EXTRA_CXXFLAGS += $(_CXX_SYSTEM_INCDIRS)
compile_commands.json: bear.d/c++ $(DEPFILES)
compile_commands.json: $(lastword $(MAKEFILE_LIST))
compile_commands.json: $(COMMON_HEADERS) $(ALL_SRCS)
	-$(MAKE) -C $(MAKEFILE_DIR) clean;
	EXTRA_CXXFLAGS='$(EXTRA_CXXFLAGS)'                     \
	  PATH="$(MAKEFILE_DIR)/bear.d/:$(PATH)"               \
	  @$(BEAR) -- $(MAKE) -C $(MAKEFILE_DIR) -j bin tests;
	EXTRA_CXXFLAGS='$(EXTRA_CXXFLAGS)'                                         \
	  PATH="$(MAKEFILE_DIR)/bear.d/:$(PATH)"                                   \
	  @$(BEAR) --append -- $(MAKE) -C $(MAKEFILE_DIR) -j pre-compiled-headers;
	@$(MAKE) -C $(MAKEFILE_DIR) clean-pch;

FULLCLEANFILES += compile_commands.json


# ---------------------------------------------------------------------------- #

# Create `compile_commands.json' and `ccls' file used for LSPs.
.PHONY: compilation-databases cdb
compilation-databases: compile_commands.json ccls
cdb: compilation-databases


# ---------------------------------------------------------------------------- #

# Run `include-what-you-use' ( wrapped )
.PHONY: iwyu
iwyu: iwyu.log

iwyu.log: compile_commands.json $(COMMON_HEADERS) $(ALL_SRCS) flake.nix
iwyu.log: flake.lock pkg-fun.nix pkgs/nlohmann_json.nix pkgs/nix/pkg-fun.nix
iwyu.log: build-aux/iwyu build-aux/iwyu-mappings.json
	@build-aux/iwyu|$(TEE) "$@";

FULLCLEANFILES += iwyu.log


# ---------------------------------------------------------------------------- #

.PHONY: lint
lint: compile_commands.json $(COMMON_HEADERS) $(ALL_SRCS)
	@$(TIDY) $(filter-out compile_commands.json,$^);


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
	exit "$$_ec";

bats-check: bin $(TEST_UTILS)
	PKGDB="$(MAKEFILE_DIR)/bin/pkgdb"                        \
	IS_SQLITE3="$(MAKEFILE_DIR)/tests/is_sqlite3"            \
	  bats --print-output-on-failure --verbose-run --timing  \
	       "$(MAKEFILE_DIR)/tests";


# ---------------------------------------------------------------------------- #

all: bin lib tests ignores
most: bin lib ignores


# ---------------------------------------------------------------------------- #

.PHONY: docs

docs: docs/index.html

docs/index.html: FORCE
	@$(DOXYGEN) ./Doxyfile

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
PC_CFLAGS += $(boost_CFLAGS)
PC_CFLAGS += $(sqlite3pp_CFLAGS)
PC_CFLAGS += -isystem $(nix_INCDIR) -include $(nix_INCDIR)/nix/config.h
PC_CFLAGS += '-DFLOX_PKGDB_VERSION=\\\\\"$(VERSION)\\\\\"'
PC_CFLAGS += '-DSEMVER_PATH=\\\\\"$(SEMVER_PATH)\\\\\"'
PC_LIBS   =  $(shell $(PKG_CONFIG) --libs-only-L nix-main) -lnixfetchers
lib/pkgconfig/flox-pkgdb.pc: $(lastword $(MAKEFILE_LIST)) $(DEPFILES)
lib/pkgconfig/flox-pkgdb.pc: lib/pkgconfig/flox-pkgdb.pc.in version
	@$(SED) -e 's,@PREFIX@,$(PREFIX),g'     \
	       -e 's,@VERSION@,$(VERSION),g'   \
	       -e 's,@CFLAGS@,$(PC_CFLAGS),g'  \
	       -e 's,@LIBS@,$(PC_LIBS),g'      \
	       $< > $@;

CLEANFILES += lib/pkgconfig/flox-pkgdb.pc

install-lib: $(LIBDIR)/pkgconfig/flox-pkgdb.pc


# ---------------------------------------------------------------------------- #

ignores: tests/.gitignore
tests/.gitignore: FORCE
	@$(MKDIR_P) $(@D);
	@printf '%s\n' $(patsubst tests/%,%,$(test_SRCS:.cc=)) > $@;


# ---------------------------------------------------------------------------- #

.PHONY: fmt
fmt: $(COMMON_HEADERS) $(ALL_SRCS)
	@$(FMT) -i $^;


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
