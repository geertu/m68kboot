#
# Makefile for common parts shared between bootstrap and LILO
#
# Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.4 1998-02-26 09:59:33 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.4  1998-02-26 09:59:33  rnhodek
# Available ETHLL_* options are machine-specific.
#
# Revision 1.3  1997/08/10 19:24:50  rnhodek
# Use bootp_mod.o only if USE_BOOTP selected.
#
# Revision 1.2  1997/07/16 09:29:25  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
# Import sources into CVS
#
#

COMMON_OBJS = linuxboot.o loadkernel.o bootinf.o stream.o file_mod.o \
              gunzip_mod.o
AMIGA_COBJS = $(addprefix amiga/,$(COMMON_OBJS))
ATARI_COBJS = $(addprefix atari/,$(COMMON_OBJS))

ifneq ($(USE_BOOTP),n)
COMMON_OBJS += bootp_mod.o
ATARI_COBJS += atari/ethlance.o
ifeq ($(MACH),atari)
BOOTOPTS += -DETHLL_LANCE
endif
ifeq ($(MACH),amiga)
BOOTOPTS += 
endif
endif

