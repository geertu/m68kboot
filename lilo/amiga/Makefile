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
# $Id: Makefile,v 1.5 2004-10-01 10:07:15 geert Exp $
#
# $Log: Makefile,v $
# Revision 1.5  2004-10-01 10:07:15  geert
# If CC is m68k-linux-gcc, LD should be m68k-linux-ld as well
#
# Revision 1.4  1998/04/09 08:07:15  rnhodek
# Define ELF_LOADER for Amiga loader sources.
#
# Revision 1.3  1998/04/06 01:40:56  dorchain
# make loader linux-elf.
# made amiga bootblock working again
# compiled, but not tested bootstrap
# loader breaks with MapOffset problem. Stack overflow?
#
# Revision 1.2  1997/09/19 09:06:50  geert
# Big bunch of changes by Geert: make things work on Amiga; cosmetic things
#
# Revision 1.1  1997/08/12 15:26:59  rnhodek
# Import of Amiga and newly written Atari lilo sources, with many mods
# to separate out common parts.
#
#

# This overrides the standard rule for .c file (native compiler) to
# use the Linux compiler. On Amiga, the loader is built as ELF.
amiga/%.o: %.c
	$(LINUX_COMPILE) -DELF_LOADER -c $< -o $@ -fpic

LD = m68k-linux-ld
LLDFLAGS = -Ttext 0


LOADER_OBJS = $(addprefix amiga/,loader.o linuxboot.o monitor.o \
                                 con_intui.o con_paula.o strlib.o\
                                 $(LOADER_COBJS))

amiga/loader: $(LOADER_OBJS) $(AMIGA_COBJS)
	$(LD) $(LLDFLAGS) -o $@ $^

