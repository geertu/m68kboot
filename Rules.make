#
# common definitions and rules for Makefiles
#
# $Id: Rules.make,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
#
# $Log: Rules.make,v $
# Revision 1.1  1997-07-15 09:45:37  rnhodek
# Initial revision
#
#

INC = -I.. -I../common -I../common/$(MACH) -I../bootstrap/$(MACH) \
      -idirafter /usr/local/m68k-linux/include \
      -idirafter /usr/m68k-linux/include -idirafter /usr/include

AMIGA_COMPILE = $(AMIGA_HOSTCC) $(AMIGA_HOSTINC) $(AMIGA_HOSTFLAGS) $(BOOTOPTS) $(INC)
ATARI_COMPILE = $(ATARI_HOSTCC) $(ATARI_HOSTINC) $(ATARI_HOSTFLAGS) $(BOOTOPTS) $(INC)

amiga/%.o: amiga/%.c
	$(AMIGA_COMPILE) -c $< -o $@

atari/%.o: atari/%.c
	$(ATARI_COMPILE) -c $< -o $@

