#
# Makefile for common parts shared between bootstrap and LILO
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.1  1997-07-15 09:45:37  rnhodek
# Initial revision
#
#

COMMON_OBJS = loadkernel.o bootinf.o stream.o file_mod.o gunzip_mod.o \
              bootp_mod.o
AMIGA_COBJS = $(addprefix amiga/,$(COMMON_OBJS))
ATARI_COBJS = $(addprefix atari/,$(COMMON_OBJS))

AMIGA_OBJS = amiga/linuxboot.o 
ATARI_OBJS = atari/linuxboot.o

ifneq ($(USE_BOOTP),n)
ATARI_OBJS += atari/ethlance.o
endif

include $(TOPDIR)/Rules.make

all: $(MACH)

amiga: amiga/liblinuxboot.a

amiga/liblinuxboot.a: $(AMIGA_OBJS) $(AMIGA_COBJS)
	$(AMIGA_HOSTAR) rcs $@ $^

atari: atari/liblinuxboot.a

atari/liblinuxboot.a: $(ATARI_OBJS) $(ATARI_COBJS)
	$(ATARI_HOSTAR) rcs $@ $^

clean:
	rm -f $(AMIGA_OBJS) $(ATARI_OBJS) $(AMIGA_COBJS) $(ATARI_COBJS) \
	  {amiga,atari}/liblinuxboot.a #*# core

dep:

$(AMIGA_COBJS): amiga/%.o: %.c
	$(AMIGA_COMPILE) -c $< -o $@

$(ATARI_COBJS): atari/%.o: %.c
	$(ATARI_COMPILE) -c $< -o $@

