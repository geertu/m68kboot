#
# Atari Makefile for lilo
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.4 1998-02-27 10:18:08 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.4  1998-02-27 10:18:08  rnhodek
# Redirect dd's output to /dev/null.
# Add strace_tos.o to object list.
#
# Revision 1.3  1998/02/24 11:14:11  rnhodek
# When building loader.map also omit ___gcc_compiled_c symbols
#
# Revision 1.2  1998/02/23 10:16:21  rnhodek
# Added rule for atari/bootos utility.
# Generate a map file for loader.tos, for debugging exception dumps.
#
# Revision 1.1  1997/08/12 15:27:07  rnhodek
# Import of Amiga and newly written Atari lilo sources, with many mods
# to separate out common parts.
#
#

.SUFFIXES:	.img .b

LOADER_OBJS = $(addprefix atari/,crt0.o bootmain.o tmpmnt.o menu.o monitor.o \
                                 strace_tos.o icon.o $(LOADER_COBJS))

atari/loader: atari/rootsect.b atari/loader.tos
	@# 1 sector from roosect.b, 1 empty sector (for mapping of booter.tos)
	@# rest is booter.tos
	cd atari; (cat rootsect.b; \
	           dd if=/dev/zero bs=512 count=1 2>/dev/null; \
	           cat loader.tos) >loader

atari/loader.tos: $(LOADER_OBJS) $(ATARI_COBJS)
	@# discard all symbols and locals
	$(ATARI_COMPILE) -s -Wl,-M -nostdlib -Wl,-x -o $@ $^ -lmint -lgem | \
	perl -ne 'printf "%08x %s\n", eval($$2), $$1 \
	          if /^  (\S+): (0x\S+)$$/ && \
			     $$1 ne "gcc2_compiled." && \
			     $$1 ne "___gnu_compiled_c";' | \
	sort >atari/loader.map

atari/bootos: atari/bootos.o
	$(CC) -o $@ $^

# rootsect.S compiled by Linux gcc
rootsect.o: rootsect.S
	$(M68K_AS) -c $< -o $@

.o.img:
	$(M68K_CC) -nostdlib -o $@ $^
	@# avoid lots of x bits being set...
	chmod -x $@

.img.b:
	objcopy --output-target=binary --strip-all -R .data -R .bss $< $@
	@# avoid lots of x bits being set...
	chmod -x $@
