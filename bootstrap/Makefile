#
# Makefile for bootstrap
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.5 2004-08-15 11:53:15 geert Exp $
#
# $Log: Makefile,v $
# Revision 1.5  2004-08-15 11:53:15  geert
# This just calls one of the binutils to correct the bits in the binary header.
# I wrote two longish mails about this subject to the mailing list so I won't
# repeat myself. It simply really clears the "Load to TTRAM flag".
# (from Petr Stehlik)
#
# Revision 1.4  1997/07/16 13:29:10  rnhodek
# Add targets to make .i and .s files; remove those files on make clean
#
# Revision 1.3  1997/07/16 10:32:54  rnhodek
# Implemented dep target; more minor Makefile changes
#
# Revision 1.2  1997/07/16 09:29:25  rnhodek
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

AMIGA_OBJS = amiga/bootstrap.o $(AMIGA_COBJS)
ATARI_OBJS = atari/bootstrap.o $(ATARI_COBJS)

SUBDEF = -DIN_BOOTSTRAP


all: $(MACH)

amiga: amiga/amiboot

amiga/amiboot: $(AMIGA_OBJS)
	$(AMIGA_COMPILE) -o $@ -s -noixemul $^

atari: atari/ataboot.ttp

atari/ataboot.ttp: $(ATARI_OBJS)
	$(ATARI_COMPILE) -o $@ $^
	m68k-atari-mint-flags --mno-fastload --mno-fastram $@

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) {amiga,atari}/*.[is] \
          {.,amiga,atari}/#*# {.,amiga,atari}/core

distclean: clean
	rm -f {amiga,atari}/.depend
	rm -f amiga/amiboot atari/ataboot.ttp

dep: clean-depend $(addprefix depend-,$($(UPCASE_MACH)_OBJS))

clean-depend: force
	rm -f $(MACH)/.depend


ifeq ($(wildcard $(MACH)/.depend),$(MACH)/.depend)
include $(MACH)/.depend
endif
