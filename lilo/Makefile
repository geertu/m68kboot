#
# Makefile for lilo
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.10 1998-04-06 01:40:54 dorchain Exp $
#
# $Log: Makefile,v $
# Revision 1.10  1998-04-06 01:40:54  dorchain
# make loader linux-elf.
# made amiga bootblock working again
# compiled, but not tested bootstrap
# loader breaks with MapOffset problem. Stack overflow?
#
# Revision 1.9  1998/03/16 10:44:27  schwab
# Make sure to build $(MACH)/tag* files first.
# Clean up clean target (replace {amiga,atari}/ by simple */.
#
# Revision 1.8  1998/03/06 09:47:07  rnhodek
# Add dumpmapfile to list of targets.
#
# Revision 1.7  1998/02/23 10:14:50  rnhodek
# Add atari/bootos to atari target list.
# rm atari/loader.map in make clean.
#
# Revision 1.6  1997/08/12 21:51:01  rnhodek
# Written last missing parts of Atari lilo and made everything compile
#
# Revision 1.5  1997/08/12 15:26:55  rnhodek
# Import of Amiga and newly written Atari lilo sources, with many mods
# to separate out common parts.
#
# Revision 1.4  1997/07/16 13:29:11  rnhodek
# Add targets to make .i and .s files; remove those files on make clean
#
# Revision 1.3  1997/07/16 10:32:54  rnhodek
# Implemented dep target; more minor Makefile changes
#
# Revision 1.2  1997/07/16 09:29:26  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
# Import sources into CVS
#
#

include $(TOPDIR)/Rules.make
include $(TOPDIR)/common/Makefile

VPATH = $(MACH):.:../common/$(MACH):../common

AMIGA_OBJS = $(AMIGA_COBJS)
ATARI_OBJS = $(ATARI_COBJS)

LOADER_COBJS = parsetags.o
LILO_COBJS   = lilo.o lilo_util.o writetags.o conf.tab.o lex.conf.o

AMIGA_LINUX_OBJS = $(addprefix amiga/,$(LILO_COBJS) bootcode.o reset.o)
ATARI_LINUX_OBJS = $(addprefix atari/,$(LILO_COBJS))

SUBDEF = -DIN_LILO

YACC = bison -d # --debug
LEX = flex
AWK = gawk

AMIGA_HOSTAS = $(AMIGA_HOSTCC) -D__ASSEMBLY__
ATARI_HOSTAS = $(ATARI_HOSTCC) -D__ASSEMBLY__
M68K_CC = m68k-linux-gcc
M68K_AS = $(M68K_CC) -D__ASSEMBLY__

all: $(MACH)/tagdef.c $(MACH)/tagdef.h $(MACH)/tagnames.c $(MACH)

amiga: amiga/loader amiga/lilo amiga/dumpmapfile

amiga/lilo: $(AMIGA_LINUX_OBJS)
	$(CC) -o $@ $^

atari: atari/loader atari/lilo atari/dumpmapfile atari/bootos

atari/lilo: $(ATARI_LINUX_OBJS)
	$(CC) -o $@ $^

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) */*.[iso] \
          \#*\# */\#*\# core */core atari/*.b atari/*.img
	rm -f atari/loader atari/loader.tos
	rm -f */conf.y */conf.tab.*
	rm -f */lex.*
	rm -f */tagdef.[ch] */tagnames.c
	rm -f atari/loader.map

distclean: clean
	rm -f */.depend
	rm -f amiga/lilo atari/lilo

clean-depend: force
	rm -f $(MACH)/.depend


$(MACH)/conf.tab.l.c: $(MACH)/conf.y
	$(YACC) -pconf -o $@ $<

$(MACH)/conf.y: conf.y $(MACH)/mach-parsefuncs.c $(MACH)/mach-rules.y \
                       parser_common.h $(MACH)/parser.h
	$(LINUX_COMPILE) -P -C -E - <$< | sed s/^@/#/ >$@

$(MACH)/lex.conf.l.c: conf.l parser_common.h $(MACH)/parser.h
	$(LEX) -Pconf -o$@ $<

$(MACH)/tagdef.c $(MACH)/tagdef.h $(MACH)/tagnames.c: tags.def $(MACH)/tags.def tags.awk
	(cat tags.def $(MACH)/tags.def) | \
      $(AWK) -f tags.awk header=$(MACH)/tagdef.h c_file=$(MACH)/tagdef.c \
             tagnames_file=$(MACH)/tagnames.c

# include machine-specific rules
ifdef MACH
include $(MACH)/Makefile
endif

dep: $(MACH)/tagdef.c $(MACH)/tagdef.h $(MACH)/tagnames.c \
	 clean-depend $(addprefix depend-,$($(UPCASE_MACH)_OBJS)) \
	 $(addprefix depend-,$(LOADER_OBJS)) \
	 $(addprefix depend-,$($(UPCASE_MACH)_LINUX_OBJS))

ifeq ($(wildcard $(MACH)/.depend),$(MACH)/.depend)
include $(MACH)/.depend
endif

