#
# Makefile for bootstrap
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.2 1997-07-16 09:29:25 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.2  1997-07-16 09:29:25  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
# Import sources into CVS
#
#

.PHONY: all amiga atari clean distclean dep

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

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) {.,amiga/atari}/#*# {.,amiga/atari}/core

distclean: clean
	rm -f amiga/amiboot atari/ataboot.ttp {.,amiga/atari}/.depend

dep:


