#
# common definitions and rules for Makefiles
#
# $Id: Rules.make,v 1.3 1997-07-16 10:32:48 rnhodek Exp $
#
# $Log: Rules.make,v $
# Revision 1.3  1997-07-16 10:32:48  rnhodek
# Implemented dep target; more minor Makefile changes
#
# Revision 1.2  1997/07/16 09:29:09  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
# Import sources into CVS
#
#

.PHONY: all amiga atari clean distclean dep force

INC = -I.. -I$(MACH) -I. -I../common/$(MACH) -I../common \
      -idirafter /usr/local/m68k-linux/include \
      -idirafter /usr/m68k-linux/include -idirafter /usr/include

AMIGA_COMPILE = $(AMIGA_HOSTCC) $(AMIGA_HOSTINC) $(AMIGA_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)
ATARI_COMPILE = $(ATARI_HOSTCC) $(ATARI_HOSTINC) $(ATARI_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)

UPCASE_MACH = $(shell echo $(MACH) | tr [a-z] [A-Z])

amiga/%.o: %.c
	$(AMIGA_COMPILE) -c $< -o $@

atari/%.o: %.c
	$(ATARI_COMPILE) -c $< -o $@

depend-amiga/%.o: %.c
	$(AMIGA_COMPILE) -MM $< >>amiga/.depend

depend-atari/%.o: %.c
	$(ATARI_COMPILE) -MM $< >>atari/.depend

