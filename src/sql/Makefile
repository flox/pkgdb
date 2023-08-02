# ============================================================================ #
#
# Generate C++ headers from SQL files.
# This basically just dumps the contents of SQL files into raw C++ strings.
#
#
# ---------------------------------------------------------------------------- #

RM   ?= rm -f
SED  ?= sed
GREP ?= grep

HEADERS =


# ---------------------------------------------------------------------------- #

.PHONY: all headers clean FORCE
.DEFAULT_GOAL = all
all: headers
clean: FORCE ; -$(RM) $(HEADERS)


# ---------------------------------------------------------------------------- #

# If file
define SQL_HH_template =
$(2:.sql=.hh): $(2)
	echo 'static const char * sql_$(1) = R"SQL('         >  $$@;  \
	$(SED) -e '/^--/d' -e '/^$$$$/d' -e '/^.read/d' $(2) >> $$@;  \
	$(GREP) -q ';' $(2) && echo 'SELECT NULL'            >> $$@;  \
	echo ')SQL";'                                        >> $$@;
HEADERS += $(2:.sql=.hh)
endef


# ---------------------------------------------------------------------------- #

$(eval $(call SQL_HH_template,input,input.sql))
$(eval $(call SQL_HH_template,packageSets,package-sets.sql))
$(eval $(call SQL_HH_template,packages,packages.sql))
$(eval $(call SQL_HH_template,tmpPaths,tmp-paths.sql))
$(eval $(call SQL_HH_template,versions,versions.sql))

headers: $(HEADERS)


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
