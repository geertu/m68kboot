#
# top-level Makefile for m68k booters
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.11 1998-02-23 10:12:48 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.11  1998-02-23 10:12:48  rnhodek
# Add ($MACH) to do_install invocation, can be set from command line if
# no /proc/hardware available.
# Fix TAGS target.
#
# Revision 1.10  1998/02/19 22:00:31  rnhodek
# Fix/rewrite check-modified and check-needed
#
# Revision 1.9  1998/02/19 21:52:13  rnhodek
# Added release, check-modified, and check-need targets.
#
# Revision 1.8  1998/02/19 21:27:06  rnhodek
# Add install and binary targets
#
# Revision 1.7  1997/08/10 19:49:46  rnhodek
# Hardwire $(CC) to m68k-linux-gcc, no sense in a cross-lilo :-)
# >& /dev/null didn't work
#
# Revision 1.6  1997/07/17 14:16:58  geert
# Use >& instead of >... 2>&1
#
# Revision 1.6  1997/07/17 14:09:34  rnhodek
# Use >& instead of >... 2>&1
#
# Revision 1.5  1997/07/16 13:39:36  rnhodek
# KERNEL_HEADERS var to find kernel headers
#
# Revision 1.4  1997/07/16 13:29:09  rnhodek
# Add targets to make .i and .s files; remove those files on make clean
#
# Revision 1.3  1997/07/16 10:32:48  rnhodek
# Implemented dep target; more minor Makefile changes
#
# Revision 1.2  1997/07/16 09:29:08  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
# Import sources into CVS
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

# where to install
PREFIX = /

# define with path to Linux/68k kernel headers, if not in the standard
# places /usr/local/m68k-linux/include/linux, /usr/m68k-linux/include/linux,
# or /usr/include/linux.
# KERNEL_HEADERS = $(HOME)/src/linux/include

ifneq ($(USE_BOOTP),n)
BOOTOPTS += -DUSE_BOOTP
endif

TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

# it doesn't make sense to build a cross-lilo, so hardwire the target arch
CC              = m68k-linux-gcc
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
        all clean distclean dep

all: amiga atari

amiga: amiga-bootstrap amiga-lilo

atari: atari-bootstrap atari-lilo

amiga-bootstrap:
	$(MAKE) -C bootstrap MACH=amiga

atari-bootstrap:
	$(MAKE) -C bootstrap MACH=atari

amiga-lilo:
	$(MAKE) -C lilo MACH=amiga

atari-lilo:
	$(MAKE) -C lilo MACH=atari

clean:
	rm -f TAGS
	$(MAKE) -C bootstrap clean
	$(MAKE) -C lilo clean

distclean:
	$(MAKE) -C bootstrap distclean
	$(MAKE) -C lilo distclean

dep:
	if $(AMIGA_HOSTCC) -v >/dev/null 2>&1; then \
		$(MAKE) -C bootstrap dep MACH=amiga; \
		$(MAKE) -C lilo dep MACH=amiga; \
	fi
	if $(ATARI_HOSTCC) -v >/dev/null 2>&1; then \
		$(MAKE) -C bootstrap dep MACH=atari; \
		$(MAKE) -C lilo dep MACH=atari; \
	fi

install:
	./do_install $(PREFIX) $(MACH)

binary:
	doit=""; [ root = "`whoami`" ] || doit=sudo; $$doit ./make_binary

release:
	@if [ "x$(VER)" = "x" ]; then \
		echo "Usage: make release VER=<release-number>"; \
		exit 1; \
	fi
	modified=`cvs status 2>/dev/null | awk '/Status:/ { if ($$4 != "Up-to-date") print $$2 }'`; \
	if [ "x$$modified" != "x" ]; then \
		echo "There are modified files: $$modified"; \
		echo "Commit first"; \
		exit 1; \
	fi
	sed "/VERSION/s/\".*\"/\"$(VER)\"/" <version.h >version.h.new
	mv version.h.new version.h
	cvs commit -m"Raised version to $(VER)" version.h
	cvs tag RELEASE-`echo $(VER) | sed 's/\./-/g'`

check-modified:
	@modified=`cvs status 2>&1 | awk 'BEGIN { OFS="" } /Examining/ { dir=$$NF } /Status:/ { if ($$4 != "Up-to-date") print dir, "/", $$2 }'`; \
	if [ "x$$modified" = "x" ]; then \
		echo "No modified files."; \
	else \
		echo "Modified files:"; \
		echo $$modified | tr ' ' '\n'; \
	fi

check-need:
	@cvs status 2>&1 | awk 'BEGIN { OFS="" } /Examining/ { dir=$$NF } /Status:/ { if ($$4 == "Needs") print dir, "/", $$2, ": ", $$4, " ", $$5 }'

bootstrap/%.i bootstrap/%.s:
	$(MAKE) -C bootstrap MACH=$(dir $(subst bootstrap/,,$@)) $(subst bootstrap/,,$@)

lilo/%.i lilo/%.s:
	$(MAKE) -C lilo MACH=$(dir $(subst lilo/,,$@)) $(subst lilo/,,$@)

TAGS:
	etags `find . -name '*.[ch]'`

tar: distclean
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



