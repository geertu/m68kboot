#
# Makefile for lilo
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.1 1997-07-15 09:45:38 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.1  1997-07-15 09:45:38  rnhodek
# Initial revision
#
#

AMIGA_OBJS = amiga/lilo.o
ATARI_OBJS = atari/lilo.o

.PHONY: all amiga atari

include $(TOPDIR)/Rules.make

all: $(MACH)

amiga: amiga-lilo

amiga-lilo: $(AMIGA_OBJS)
	$(AMIGA_COMPILE) -o $@ -s -noixemul $(AMIGA_OBJS) ../common/amiga/liblinuxboot.a

atari: atari-lilo

atari-lilo: $(ATARI_OBJS)
	$(ATARI_COMPILE) -o $@ $(ATARI_OBJS) ../common/atari/liblinuxboot.a

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) #*# core

dep:


$(AMIGA_OBJS): amiga/%.o: amiga/%.c
	$(AMIGA_COMPILE) -c $< -o $@

$(ATARI_OBJS): atari/%.o: atari/%.c
	$(ATARI_COMPILE) -c $< -o $@

