#
# top-level Makefile for m68k booters
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

# possible options:
#   AOUT_KERNEL:
#     Include support for a.out kernels (may go away in future!)
#   BOOTINFO_COMPAT_1_0:
#     Include support for booting kernel with bootinfo version 1.0 (up
#     to 2.0.x)
#
BOOTOPTS  = -DBOOTINFO_COMPAT_1_0 # -DAOUT_KERNEL

USE_BOOTP = n
ifneq ($(USE_BOOTP),n)
BOOTOPTS += -DUSE_BOOTP
endif

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

CC              = gcc
CFLAGS          = -O2 -fomit-frame-pointer -Wall

AMIGA_HOSTCC    = m68k-cbm-amigados-gcc
AMIGA_HOSTINC   = 
AMIGA_HOSTFLAGS = -m68030 -O2 -Wall -Dlinux
AMIGA_HOSTAR    = m68k-cbm-amigados-ar

ATARI_HOSTCC    = m68k-mint-gcc
ATARI_HOSTINC   = 
ATARI_HOSTFLAGS = -m68030 -m68881 -Dlinux -O2 -fomit-frame-pointer \
                  -Wall
ATARI_HOSTAR    = m68k-mint-ar

.EXPORT_ALL_VARIABLES:

.PHONY: amiga amiga-bootstrap amiga-lilo amiga-common \
        atari atari-bootstrap atari-lilo atari-common \
        all clean dep

all: amiga atari

amiga: amiga-bootstrap amiga-lilo

atari: atari-bootstrap atari-lilo

amiga-bootstrap: amiga-common
	$(MAKE) -C bootstrap MACH=amiga

atari-bootstrap: atari-common
	$(MAKE) -C bootstrap MACH=atari

amiga-lilo: amiga-common
	$(MAKE) -C lilo MACH=amiga

atari-lilo: atari-common
	$(MAKE) -C lilo MACH=atari

amiga-common:
	$(MAKE) -C common MACH=amiga

atari-common:
	$(MAKE) -C common MACH=atari

clean:
	$(MAKE) -C common clean
	$(MAKE) -C bootstrap clean
	$(MAKE) -C lilo clean

dep:
	$(MAKE) -C common dep
	$(MAKE) -C bootstrap dep
	$(MAKE) -C lilo dep

TAGS: etags `find . -name '*.[ch]'`

tar: clean
	cd ..; \
	name="$(notdir $(shell pwd))"; \
	namev="$$name-$(shell perl -ne 'print "$$1\n" if /VERSION.*"(\S+)"/;' version.h)"; \
	mv $$name $$namev; \
	tar cf $$namev.tar `find $$namev -path $$namev/CVS -prune -o -type f -print`; \
	gzip -9f $$namev.tar; \
	mv $$namev $$name

diff: distclean
	@if [ "x$(OLDVER)" = "x" ]; then \
		echo "Usage: make diff OLDVER=<last-release-tag>"; \
		exit 1; \
	fi; \
	name="$(notdir $(shell pwd))"; \
	namev="$$name-$(shell perl -ne 'print "$$1\n" if /VERSION.*"(\S+)"/;' version.h)"; \
	cvs diff -u -rRELEASE-$(OLDVER) >../$$namev.diff; \
	gzip -9f ../$$namev.diff

ifeq ($(wildcard .depend),.depend)
include .depend
endif


