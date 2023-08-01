# ============================================================================ #
#
# Generate C++ headers from SQL files.
# This basically just dumps the contents of SQL files into raw C++ strings.
#
#
# ---------------------------------------------------------------------------- #

RM  ?= rm -f
SED ?= sed

HEADERS ::=


# ---------------------------------------------------------------------------- #

.PHONY: all headers clean FORCE
.DEFAULT_GOAL = all
all: headers


# ---------------------------------------------------------------------------- #

define SQL_HH_template =
$(2:.sql=.hh): $(2)
	echo 'static const char * sql_$(1) = R"SQL(' >  $$@
	$(SED) -e '/^--/d' -e '/^$$$$/d' $(2)        >> $$@
	echo ')SQL";'                                >> $$@
HEADERS += $(2:.sql=.hh)
endef


# ---------------------------------------------------------------------------- #

$(eval $(call SQL_HH_template,input,input.sql))
$(eval $(call SQL_HH_template,packageSets,package-sets.sql))
$(eval $(call SQL_HH_template,packages,packages.sql))
$(eval $(call SQL_HH_template,tmpPaths,tmp-paths.sql))
$(eval $(call SQL_HH_template,versions,versions.sql))

headers: $(HEADERS)
clean: FORCE ; -$(RM) $(HEADERS)


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
