#
# Amiga Makefile for lilo
#
# Copyright (c) 1997 by Geert Uytterhoeven
#                       Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.1 1997-08-12 15:26:59 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.1  1997-08-12 15:26:59  rnhodek
# Import of Amiga and newly written Atari lilo sources, with many mods
# to separate out common parts.
#
#

LOADER_OBJS = $(addprefix amiga/,loader.o linuxboot.o monitor.o \
                                 con_intui.o con_paula.o \
                                 $(LOADER_COBJS))

amiga/loader: $(LOADER_OBJS) $(AMIGA_COBJS)
	$(AMIGA_COMPILE) -noixemul -s -o $@ $^

