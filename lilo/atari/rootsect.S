/* rootsect.S -- Root sector boot code of Atari LILO
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: rootsect.S,v 1.5 1998-03-12 09:26:51 rnhodek Exp $
 * 
 * $Log: rootsect.S,v $
 * Revision 1.5  1998-03-12 09:26:51  rnhodek
 * In aprint_error, save error number (in %d0) around call to Mfree.
 *
 * Revision 1.4  1998/03/06 09:51:32  rnhodek
 * Implement new option skip-on-keys: New field 'modif_mask' which
 * contains a mask to and the Kbshift() return value with.
 *
 * Revision 1.3  1998/02/23 10:26:27  rnhodek
 * Load address, not contents, of lilo_string to %a3
 * Start of crt0.S is at offset 1536 (not 512) bytes of malloc-ed area,
 * since bootblock and map sector are loaded again.
 * Fix/extend some comments.
 *
 * Revision 1.2  1998/02/19 20:39:42  rnhodek
 * Fix wrong registers
 *
 * Revision 1.1  1997/08/12 15:27:12  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include "bootparam.h"
	
/* register saved while in the boot loader */
#define SAVE_REGS	%d0-%d7/%a0-%a6

/* system variables */
_ill_vec	= 0x10
_bootdev	= 0x446
_drvbits	= 0x4c2
_sysbase	= 0x4f2
osdate		= 0x18

/* all stuff in text segment... */
	.text
	
/* jump to start of boot code (first two bytes also copied on installtion)
 * (_start is to shut up the linker, otherwise complaining about a
 * missing entry symbol)
 */
	.globl _start	
_start:
	bras	start

/* Skip MS-DOS boot sector data (not overwritten on installation!)
 * The end of that BIOS parameter block is taken as 0x24, not 0x1e as
 * usual, because some MS-DOS versions use the additional 6 bytes.
 */
	.org 0x24

/* also LILO Magic */
lilo_string:
	.ascii	"LILO"	
	
/* Data passed to the boot loader: */

boot_device:
	.word	0		/* device from which to load the next sector,
				 * in DMAread format; special is -1, which
				 * stands for the current floppy device in
				 * _bootdev. This is from where the ROM loaded
				 * the floppy boot sector. */
modif_mask:
	.word	0		/* modifier mask on which to skip Lilo */
	
#ifdef SECONDARY_ROOTSEC_NEEDED
next_sector:
	.long	0		/* number of secondary root sector */
#endif

map_sector:
	.long	0		/* number of map sector */

		
/* Here now is the real code: */
start:
	movml	SAVE_REGS,%sp@-

/* Remove disk devices and reset _bootdev to A:, to clean up from previous
 * boots if we're returning to ROM without booting anything ourselves.
 */
	andl	#3,_drvbits
	movw	_bootdev,%d7		/* save current _bootdev for later */
	clrw	_bootdev
	
/* Test for ALT being pressed, and if yes, exit immediately. This is
 * usually done only by root sector code, to give users a chance to
 * avoid booting from hard disk, but we may have been installed in the
 * master root sector...
 */

	movl	#(11<<16)+0xffff,%sp@-
	trap	#13			/* Kbshift(-1) */
	addql	#4,%sp
	andw	pc@(modif_mask),%d0
	jbne	back_to_rom

	lea	%pc@(lilo_string),%a3
/* Output an 'L' to indicate LILO started booting */
	jbsr	putlc
	
	movw	%pc@(boot_device),%d4
	movq	#1,%d5

#ifdef SECONDARY_ROOTSEC_NEEDED

/* This is the code to load a second root sector just after this one. It
 * showed that the space in the master root sector is sufficient, the the code
 * is commented out.
 */
	
/* Load the next loader sector immediately after this code. The device
 * and location has been patched by the installer into boot_device and
 * next_sector.
 */
	lea	%pc@(second_part),%a4
	movl	%pc@(next_sector),%d6
	jbsr	load_sector
	jbne	print_error

#endif /* SECONDARY_ROOTSEC_NEEDED */
	
/* Output an 'I' to indicate we've loaded the second half */
	jbsr	putlc

/* Allocate the largest free memory block for booter.tos */
	movq	#-1,%d0
	movl	%d0,%sp@-
	movw	#72,%sp@-
	trap	#1			/* Malloc(-1): get biggest block */
	movl	%d0,%sp@(2)
	trap	#1			/* Malloc(d0): alloc that block */
	addql	#6,%sp
	tstl	%d0
	jbeq	back_to_rom		/* no memory (shouldn't happen) */
	movl	%d0,%a6			/* a6: pointer to block (const) */
	movl	%d0,%a5			/* a5: map table pointer */

/* Output another 'L' to indicate we've allocated our memory */
	jbsr	putlc
	
/* now load the map sector to the start of the allocated area
 * (%d5 (number of sectors) and %d4 (device) are still valid from the last
 * call to load_sector)
 */
	movl	%a5,%a4
	movl	%pc@(map_sector),%d6
	jbsr	load_sector
	jbne	aprint_error
	addaw	#512,%a4

/* Output an 'O' to indicate that we got the map sector */
	jbsr	putlc

/* load all the sector chunks the map sector tells us
 * (%d4 (device) still valid, %a4 set up just above)
 */
	movw	#N_MAPENTRIES-2,%d3
	addql	#6,%a5			/* skip first entry in map sector,
					 * it's the file length */
load_loop:	
	movl	%a5@+,%d6		/* sector number */
	movq	#0,%d5
	movw	%a5@+,%d5		/* number of sectors */
	jbeq	load_end
	jbsr	load_sector
	jbne	aprint_error
	lsll	#8,%d5
	lsll	#1,%d5
	addl	%d5,%a4			/* add %d5 * 512 to load address */
	movq	#'.',%d1		/* for output */
	jbsr	putc			/* print a '.' for each chunk */
	dbra	%d3,load_loop
load_end:
	
#ifdef SECONDARY_ROOTSEC_NEEDED
	/* continued in second sector... */
	jbra	second_part
#else
	/* Jump to start of loaded program
	 * (this is 3*SECTOR_SIZE behind the load start, since the first
	 * 512 byte are occupied by the map sector, and 1024 are the
	 * bootsector and the map sector again, because those are mapped at
	 * the start of loader.patched!
	 */
	jbra	%a6@(1536)
#endif		


/* Print next character in "LILO", %a3 is pointer into string
 * clobbered: %d1
 */
putlc:
	movb	%a3@+,%d1
	/* fall through to putc */
			
/* Print a character in %d1
 * clobbered: nothing
 */
putc:
	movml	%d0-%d2/%a0-%a2,%sp@-
	movw	%d1,%sp@-
	movw	#2,%sp@-
	trap	#1			/* Cconout */
	addql	#4,%sp
	movml	%sp@+,%d0-%d2/%a0-%a2
	rts

/* free memory pointed to by %a6, then print error number and exit */
aprint_error:
	movl	%d0,sp@-		/* save error number */
	movl	%a6,%sp@-
	movw	#73,%sp@-
	trap	#1
	addql	#6,%sp
	movl	sp@+,%d0
	/* fall through to print_error */

/* Print the error number in %d0 and a newline, then exit */
print_error:
	negl	%d0			/* make positive */
	divuw	#10,%d0			/* dividend in low word, remainder
					 * in high word of %d0 */
	addl	#0x300030,%d0		/* make ASCII digits out of them */
	movq	#'-',%d1
	jbsr	putc
	movl	%d0,%d1
	jbsr	putc
	swap	%d1
	jbsr	putc
	movq	#13,%d1
	jbsr	putc
	movq	#10,%d1
	jbsr	putc
	/* fall through to back_to_rom */

/* Jump back to ROM code, which will try to boot from other devices,
 * or lets the hd boot finally fail
 */
back_to_rom:
	movml	%sp@+,SAVE_REGS
	rts

/* Read a sector from disk with DMAread()
 *
 * Parameters:
 *   %d6: sector number
 *   %d5: number of sectors
 *   %d4: device number
 *   %a4: destination address
 * Return values:
 *   %d0: BIOS error code (0 = OK)
 *   %sr: set for %d0
 * Clobbered registers:
 *   %d1,%d2,%a0-%a2 (%d0 is retval)
 *
 * If the device number is positive or 0, it's a DMAread number: 0..7 are ACSI
 * devices with IDs 0..7, 8..15 are ACSI devices with IDs 0..7, and 16 and 17
 * are the two disks. (Unfortunately, current DMAread implementations don't
 * support the second disk :-(( ) If it's negative, the value of %d7 is used
 * for Rwabs(). Rationale: If this code resides in a floppy bootsector, and
 * the kernel should be loaded from the floppy, too, we can not used a fixed
 * device number (floppy A: or B:), but must use the floppy drive from which
 * the boot sector was loaded. This device number was in _bootdev and has been
 * saved to %d7 before clearing _bootdev.
 *
 * We don't supply here a special function for reading from ACSI, since all
 * machines Linux runs on have a TOS version >= 2.0 and therefore have DMAread.
 */
load_sector:
	movw	%d4,%sp@-		/* device */
	jbmi	1f
	movl	%a4,%sp@-		/* buffer address */
	movw	%d5,%sp@-		/* number of sectors */
	movl	%d6,%sp@-		/* sector */
	movw	#0x2a,%sp@-
	trap	#14			/* call DMAread() */
	jbra	2f
1:	movw	%d7,%sp@		/* _bootdev device number for Rwabs */
	movw	%d6,%sp@-		/* sector (16 bit here, but ok for
					 * floppies... */
	movw	%d5,%sp@-		/* number of sectors */
	movl	%a4,%sp@-		/* buffer address */
	clrw	%sp@-			/* flag==0: read */		
	movw	#4,%sp@-
	trap	#13			/* call Rwabs() */
2:	addw	#14,%sp
	tstl	%d0
	rts

#if 0
/*
 * With all known ROM versions, it isn't necessary to push and invalidate the
 * caches. Atari ROMs completely disable caches on reset, and only GEM enables
 * them at startup (if the menu item is checked). Other patched ROMs (Medusa,
 * ...) only enable the insn cache, not data caches. So we don't have to push.
 * The only case where a cache action would be required is if the loaded code
 * would already be in the insn cache. But this is very unlikely, since the
 * boot sector code is the first code executed from RAM at all... So I saved
 * flushing the cache, but left in the code here if it should become necessary
 * in future.
 */
/* 
 * Push data cache and invalide insn cache.
 * I don't bother to detect the CPU type here, just let illegal insn
 * expections jump around parts that don't work.
 */
flush_cache:
	movl	%sp,%a1			/* save sp, to throw away stack frames
					 * created by exceptions */
	movl	_ill_vec,%a0
	movl	%pc@(1f),_ill_vec
	nop
	.chip	68040
	cpusha	%bc			/* '040 and '060 part */
	.chip	68k
	nop
1:	movl	%pc@(1f),_ill_vec
	.chip	68030
	movec	%cacr,%d0		/* '030 part */
	orw	#0x808,%d0
	movec	%d0,%cacr
	.chip	68k
1:	movl	%a0,_ill_vec
	movl	%a1,%sp
	rts
#endif		

	/* skip to partition table */
	.org 0x156
	.skip 8 * 12,1	/* 8 ICD partition entries */
	
	.org 0x1c2
	.skip 4		/* hard disk size (long) */
	.skip 4 * 12,2	/* 4 partition entries */
	.skip 10	/* bad sector list, checksum field */

#ifdef SECONDARY_ROOTSEC_NEEDED
second_part:
/* Start of code of second root sector; currently not needed */
#endif	

/* Local Variables:			*/
/*   tab-width:	8			*/
/*   fill-column: 78			*/
/*   asm-comment-char: ?|		*/
/* End:					*/
