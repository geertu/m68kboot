#
# common definitions and rules for Makefiles
#
# $Id: Rules.make,v 1.2 1997-07-16 09:29:09 rnhodek Exp $
#
# $Log: Rules.make,v $
# Revision 1.2  1997-07-16 09:29:09  rnhodek
# Reorganized Makefiles so that all objects are built in
# {bootstrap,lilo}/{amiga,atari}, not in common anymore. Define IN_BOOTSTRAP or
# IN_LILO so that common sources can distinguish between the environments.
# Other minor Makefile changes.
#
# Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
# Import sources into CVS
#
#

INC = -I.. -I$(MACH) -I. -I../common/$(MACH) -I../common \
      -idirafter /usr/local/m68k-linux/include \
      -idirafter /usr/m68k-linux/include -idirafter /usr/include

AMIGA_COMPILE = $(AMIGA_HOSTCC) $(AMIGA_HOSTINC) $(AMIGA_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)
ATARI_COMPILE = $(ATARI_HOSTCC) $(ATARI_HOSTINC) $(ATARI_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)

amiga/%.o: %.c
	$(AMIGA_COMPILE) -c $< -o $@

atari/%.o: %.c
	$(ATARI_COMPILE) -c $< -o $@

