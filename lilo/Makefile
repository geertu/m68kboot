#
# Makefile for lilo
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.3 1997-07-16 10:32:54 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.3  1997-07-16 10:32:54  rnhodek
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

AMIGA_OBJS = amiga/lilo.o $(AMIGA_COBJS)
ATARI_OBJS = atari/lilo.o $(ATARI_COBJS)

SUBDEF = -DIN_LILO


all: $(MACH)

amiga: amiga/lilo

amiga/lilo: $(AMIGA_OBJS)
	$(AMIGA_COMPILE) -o $@ -fl libnix -s $^ -lnix

atari: atari/lilo

atari/lilo: $(ATARI_OBJS)
	$(ATARI_COMPILE) -o $@ $^

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) {.,amiga,atari}/#*# {.,amiga,atari}/core

distclean: clean
	rm -f {amiga,atari}/.depend
	rm -f amiga/lilo atari/lilo

dep: clean-depend $(addprefix depend-,$($(UPCASE_MACH)_OBJS))

clean-depend: force
	rm -f $(MACH)/.depend


ifeq ($(wildcard $(MACH)/.depend),$(MACH)/.depend)
include $(MACH)/.depend
endif

