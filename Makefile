all::

FIND=find
SED=sed
XARGS=xargs
CAT=cat
MKDIR=mkdir -p
CP=cp
CMP=cmp -s
HTML2TXT=lynx -dump
MARKDOWN=markdown

VWORDS:=$(shell src/getversion.sh --prefix=v MAJOR MINOR)
VERSION:=$(word 1,$(VWORDS))
BUILD:=$(word 2,$(VWORDS))

## Provide a version of $(abspath) that can cope with spaces in the
## current directory.
myblank:=
myspace:=$(myblank) $(myblank)
MYCURDIR:=$(subst $(myspace),\$(myspace),$(CURDIR)/)
MYABSPATH=$(foreach f,$1,$(if $(patsubst /%,,$f),$(MYCURDIR)$f,$f))

-include $(call MYABSPATH,config.mk)
-include nonowimp-env.mk

CPPFLAGS += -DVERSION='"$(file <VERSION)"'

test_binaries.c += nonowimp
nonowimp_obj += fileio
nonowimp_obj += infobox
nonowimp_obj += main
nonowimp_obj += menu
nonowimp_obj += nonowin
nonowimp_obj += state
nonowimp_obj += version

nonowimp_lib += -lnonogram
nonowimp_lib += -lyac_dsktop
nonowimp_lib += -lyac_vector
nonowimp_lib += -lyac_vdu
nonowimp_lib += -lyac_wimp
nonowimp_lib += -lyac_sprite
nonowimp_lib += -lyac_font
nonowimp_lib += -lyac_draw
nonowimp_lib += -lyac_os
nonowimp_lib += -ledges

#TEXTFILES += HISTORY
TEXTFILES += VERSION
TEXTFILES += COPYING
TEXTFILES += Guide

SOURCES:=$(filter-out $(headers),$(shell $(FIND) src/obj \( -name "*.c" -o -name "*.h" -o -name "*.hh" \) -printf '%P\n'))

riscos_zips += nonowimp
nonowimp_appname=!Nonogram
nonowimp_rof += !Boot,feb
nonowimp_rof += !Help,feb
nonowimp_rof += !RO2Help,feb
nonowimp_rof += !Run,feb
nonowimp_rof += !Sprites,ff9
nonowimp_rof += !Sprites22,ff9
nonowimp_rof += Messages,fff
nonowimp_rof += Templates,fec
nonowimp_rof += $(TEXTFILES:%=Docs/%,fff)
nonowimp_rof += $(call riscos_src,$(SOURCES))
nonowimp_runimage=nonowimp

include binodeps.mk

tmp/obj/version.o: VERSION

# tmp/README.html: README.md
# 	$(MKDIR) '$(@D)'
# 	$(MARKDOWN) '$<' > '$@'

# $(BINODEPS_OUTDIR)/riscos/!Nonogram/Docs/Guide,fff: tmp/README.html
# 	$(MKDIR) "$(@D)"
# 	$(HTML2TXT) "$<" > "$@"

$(BINODEPS_OUTDIR)/riscos/!Nonogram/Docs/VERSION,fff: VERSION
	$(MKDIR) "$(@D)"
	$(CP) "$<" "$@"

$(BINODEPS_OUTDIR)/riscos/!Nonogram/Docs/COPYING,fff: LICENSE.txt
	$(MKDIR) "$(@D)"
	$(CP) "$<" "$@"


all:: VERSION BUILD riscos-zips

MYCMPCP=$(CMP) -s '$1' '$2' || $(CP) '$1' '$2'
.PHONY: prepare-version
mktmp:
	@$(MKDIR) tmp/
prepare-version: mktmp
	$(file >tmp/BUILD,$(BUILD))
	$(file >tmp/VERSION,$(VERSION))
BUILD: prepare-version
	@$(call MYCMPCP,tmp/BUILD,$@)
VERSION: prepare-version
	@$(call MYCMPCP,tmp/VERSION,$@)

install:: install-riscos

tidy::
	$(FIND) . -name "*~" -delete

distclean: blank
	$(RM) VERSION BUILD

# Set this to the comma-separated list of years that should appear in
# the licence.  Do not use characters other than [0-9,] - no spaces.
YEARS=2000-7,2012

update-licence:
	$(FIND) . -name ".git" -prune -or -type f -print0 | $(XARGS) -0 \
	$(SED) -i 's/Copyright (C) [0-9,-]\+  Steven Simpson/Copyright (C) $(YEARS)  Steven Simpson/g'
