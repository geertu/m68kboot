#
# common definitions and rules for Makefiles
#
# $Id: Rules.make,v 1.9 1998-02-24 11:10:23 rnhodek Exp $
#
# $Log: Rules.make,v $
# Revision 1.9  1998-02-24 11:10:23  rnhodek
# Add -DMACHNAME to LINUX_COMPILE
#
# Revision 1.8  1998/02/19 20:40:12  rnhodek
# Make things compile again
#
# Revision 1.7  1997/08/10 19:24:20  rnhodek
# In depend- rules, write directory prefix in rules.
# Added depend- rules for .S Linux files.
#
# Revision 1.6  1997/07/30 21:40:52  rnhodek
# Added rules for assembler and Linux sources
#
# Revision 1.5  1997/07/16 13:39:36  rnhodek
# KERNEL_HEADERS var to find kernel headers
#
# Revision 1.4  1997/07/16 13:29:10  rnhodek
# Add targets to make .i and .s files; remove those files on make clean
#
# Revision 1.3  1997/07/16 10:32:48  rnhodek
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

INC = -I.. -I$(MACH) -I. -I../common/$(MACH) -I../common
ifdef KERNEL_HEADERS
INC += -I$(KERNEL_HEADERS)
endif
INC += -idirafter /usr/local/m68k-linux/include \
       -idirafter /usr/m68k-linux/include -idirafter /usr/include

AMIGA_COMPILE = $(AMIGA_HOSTCC) $(AMIGA_HOSTINC) $(AMIGA_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)
ATARI_COMPILE = $(ATARI_HOSTCC) $(ATARI_HOSTINC) $(ATARI_HOSTFLAGS) \
                $(BOOTOPTS) $(INC) $(SUBDEF)
LINUX_COMPILE = $(CC) $(CFLAGS) $(BOOTOPTS) $(INC) $(SUBDEF) \
			    -DMACHNAME=\"$(MACH)\"

UPCASE_MACH = $(shell echo $(MACH) | tr [a-z] [A-Z])

# native OS .c or .S -> .o
amiga/%.o: %.c
	$(AMIGA_COMPILE) -c $< -o $@

atari/%.o: %.c
	$(ATARI_COMPILE) -c $< -o $@

amiga/%.o: %.S
	$(AMIGA_COMPILE) -D__ASSEMBLY__ -c $< -o $@

atari/%.o: %.S
	$(ATARI_COMPILE) -D__ASSEMBLY__ -c $< -o $@

# native OS .c -> .i or .s
amiga/%.i: %.c
	$(AMIGA_COMPILE) -E -dD $< -o $@

atari/%.i: %.c
	$(ATARI_COMPILE) -E -dD $< -o $@

amiga/%.s: %.c
	$(AMIGA_COMPILE) -S $< -o $@

atari/%.s: %.c
	$(ATARI_COMPILE) -S $< -o $@

# Linux .c -> .o
amiga/%.o atari/%.o: %.l.c
	$(LINUX_COMPILE) -c $< -o $@

amiga/%.o atari/%.o: %.l.S
	$(LINUX_COMPILE) -D__ASSEMBLY__ -c $< -o $@

# special targets for building dependencies
depend-amiga/%.o: %.c
	@echo -n "amiga/" >>amiga/.depend
	$(AMIGA_COMPILE) -MM $< >>amiga/.depend

depend-amiga/%.o: %.l.c
	@echo -n "amiga/" >>amiga/.depend
	$(LINUX_COMPILE) -MM $< >>amiga/.depend

depend-amiga/%.o: %.S
	@echo -n "amiga/" >>amiga/.depend
	$(LINUX_COMPILE) -MM $< >>amiga/.depend

depend-amiga/%.o: %.l.S
	@echo -n "amiga/" >>amiga/.depend
	$(LINUX_COMPILE) -MM $< >>amiga/.depend

depend-atari/%.o: %.c
	@echo -n "atari/" >>atari/.depend
	$(ATARI_COMPILE) -MM $< >>atari/.depend

depend-atari/%.o: %.l.c
	@echo -n "atari/" >>atari/.depend
	$(LINUX_COMPILE) -MM $< >>atari/.depend

depend-atari/%.o: %.S
	@echo -n "atari/" >>atari/.depend
	$(LINUX_COMPILE) -MM $< >>atari/.depend

depend-atari/%.o: %.l.S
	@echo -n "atari/" >>atari/.depend
	$(LINUX_COMPILE) -MM $< >>atari/.depend
