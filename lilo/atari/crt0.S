/* crt0.S -- Startup code for TOS part of Atari LILO booter
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: crt0.S,v 1.8 1998-03-16 10:47:26 schwab Exp $
 * 
 * $Log: crt0.S,v $
 * Revision 1.8  1998-03-16 10:47:26  schwab
 * Call Dsetdrv(_bootdev) on exit_action==1, since setting the current drive from
 * the loader process is lost on Pterm!
 * Do not trap execption #5 (division by zero).
 *
 * Revision 1.7  1998/03/02 13:08:29  rnhodek
 * Print current basepage on exceptions, not basepage of loader.
 * Fix indexing of 'framesize' array (d1 is .w, not .l)
 * Also print USP on exceptions.
 * Add some '040 and '060 frame sizes to table.
 *
 * Revision 1.6  1998/02/27 10:20:33  rnhodek
 * Save system stack contents (before call of root sector) and restore
 * them before returning to ROM, because GEMDOS uses the same memory as
 * default supervisor stack for new programs.
 * Don't keep our exception debugging after the loader exited (was for debugging).
 *
 * Revision 1.5  1998/02/26 10:19:03  rnhodek
 * Don't trap Line-A vector with exc_general, it's used by the VDI :-)
 * Let BP_ENV point to space after the basepage; removed dummy_env.
 * For now, don't restore the vector table so that we can debug errors
 * after the loader exited.
 * exc_general: vector number in stack frame is already offset, no need
 * to << 2.
 *
 * Revision 1.4  1998/02/25 10:34:55  rnhodek
 * Add missing '#' before constant STACK_SIZE.
 * Also include TOSPRGHDR_SIZE in _PgmSize.
 * Also dump 4 stack longs before current SP.
 *
 * Revision 1.3  1998/02/24 11:20:55  rnhodek
 * Save old vectors before modifying them, and restore them after loader
 * exits. Otherwise, they would point to unallocated memory afterwards.
 * Initialize all standard handles to -1 (console).
 * Print SSW as word.
 * Additionally print SR.
 *
 * Revision 1.2  1998/02/23 10:22:52  rnhodek
 * Many changes to make it work:
 *  - Also provide variable _environ, needed by getenv().
 *  - Moved definition of CurrentFloppy to here from bootmain.c
 *  - Fix size of stuff before loader from 512 to 1536.
 *  - #define for STACK_SIZE
 *  - Fill in more fields of the basepage, particularily BP_DEVX;
 *    otherwise stdout doesn't work. (Such stuff is normally done by
 *    Pexec(3), but we don't use that.)
 *  - Clear bss of TOS program, to avoid uninitialized variables.
 *  - After the Pexec, go to supervisor mode again.
 *  - Change Cconout to Bcconout in puts, since exceptions can also
 *    happen inside GEMDOS, and that is not reentrant.
 * Added code for debugging exceptions in the loader. All common
 * exceptions are caught and register/stack dumps are printed instead of
 * bombs.
 *
 * Revision 1.1  1997/08/12 15:27:08  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include "bootparam.h"

#define _sysbase	0x4f2
#define _bootdev	0x446
#define STACK_SIZE	8192
#define DEBUG_EXCEPTIONS

			
/*
 * Globally visible variables declared by crtinit.c
 */
	
	.bss
	.globl	__app
	.globl	__base
	.globl	__heapbase	| picked up from user or from heapbase.c
	.globl	__stksize	| long, declared by user or in stksiz.c
	.globl	__PgmSize
	.globl	___DEFAULT_BUFSIZ__
	.globl	__split_mem
	.globl	_environ
	
	.comm	__app, 2
	.comm	__base, 4
	.comm	__PgmSize, 4
	.comm	___DEFAULT_BUFSIZ__, 4
	.comm	__split_mem, 2
	.comm	_environ, 4

#define BUFSIZ	1024
	
/*
 * externs to pull ident strings of all used libraries into the
 * executable; if a library is not used, then the extern is
 * satisfied by a dummy in the library
 */

	.globl	___Ident_libg
	.globl	___Ident_curses
	.globl	___Ident_widget
	.globl	___Ident_gem
	.globl	___Ident_pml
	.globl	___Ident_gnulib

/* Dummy argument array and environment */

	.data
dummy_environ:
	.long	0
arg_array:
	.long	arg0
	.long	0
arg0:
	.asciz	"loader.ttp"
_CurrentFloppy:
	.long	0
		
/* own data */

halt_text:
	.ascii	"LILO loader exited -- system halted."
	.byte	13,10,0

	.bss
	.comm	stack_save_area,76
		
/*
 * used globals
 */

	.globl	_ExitAction
	.globl	_CurrentFloppy
	.globl	_exit
	
/*
 * Code
 */

	.text
	.even
	.globl	__start
__start:

/* Output a 'b' to indicate the TOS prg part has started up */
	moveq	#'b',d0
	jbsr	putc

	/* First relocate ourselves */
	
	lea	pc@(__start),a0		/* a0: program base addr */
	lea	a0@(-TOSPRGHDR_SIZE),a6	/* a6: pointer to header */
	movl	a0,a2
	movl	a0,d0			/* d0: reloc offset */
	addl	a6@(PRGHDR_TLEN),a2
	addl	a6@(PRGHDR_DLEN),a2
	addl	a6@(PRGHDR_SLEN),a2	/* a2: current reloc entry */
	tstl	a2@
	jbeq	end_reloc		/* first long is zero: no relocs */
	movq	#0,d1			/* clear d1 for sign extends */
	addl	a2@+,a0			/* go to first reloc addr */
do_reloc:	
	addl	d0,a0@			/* relocate (a0) */
fetch_next_reloc:
	movb	a2@+,d1			/* fetch offset to next reloc */
	jbeq	end_reloc		/* zero byte means end */
	cmpb	#1,d1
	jbne	1f
	addw	#0xfe,a0		/* one byte means add 254 and next */
	jbra	fetch_next_reloc
1:	addw	d1,a0
	jbra	do_reloc
end_reloc:
			
/* Output a 'o' to indicate the TOS prg is relocated */
	moveq	#'o',d0
	jbsr	putc

#ifdef DEBUG_EXCEPTIONS
	/* save old vectors */
	lea	0x8,a0
	lea	save_vecs,a1
	movq	#13,d0
1:	movl	a0@+,a1@+
	dbra	d0,1b
	/* overwrite some vectors */	
	movl	#exc_buserr,0x8
	movl	#exc_addrerr,0xc
	movl	#exc_general,0x10
	movl	#exc_general,0x18
	movl	#exc_general,0x1c
	movl	#exc_general,0x20
	movl	#exc_general,0x24
	movl	#exc_general,0x2c
	movl	#exc_general,0x34
	movl	#exc_general,0x38
	movl	#exc_general,0x3c
#endif
	
	/* save value of d7 (old contents of _bootdev) */
	andl	#0xffff,d7
	movl	d7,_CurrentFloppy
	/* set up desired size of loader stack */

	/* Since we're not started by TOS, we've got no real basepage.
	 * However, we need __base for Mshrink()ing our memory. The base
	 * address of our memory block is the start address, minus the size of
	 * the TOS program header, minus 1536 bytes where the map sector was
	 * stored.
	 */
	movl	#__start-TOSPRGHDR_SIZE-1536,a5

	/* calculate final program size and do the Mshrink() */
	movl	a6@(PRGHDR_TLEN),d7
	addl	a6@(PRGHDR_DLEN),d7
	addl	a6@(PRGHDR_BLEN),d7
	addl	#1536+TOSPRGHDR_SIZE,d7	/* junk before TOS program */
	addl	#STACK_SIZE,d7		/* length of text+data+BSS+stack */
	
	movl	d7,sp@-			/* new size of block */
	movl	a5,sp@-			/* base address */
	clrw	sp@-			/* dummy arg */
	movw	#74,sp@-
	trap	#1			/* Mshrink() */
	addw	#12,sp

	/* Create a dummy basepage in the space before our code
	 * (where the map sector was). We run the loader as a separate
	 * process so that all allocated memory will be cleaned up
	 * after it finishes. We can't use Pexec(5) for this, because
	 * this allocates the biggest free memory block and an
	 * environment.
	 */
	movl	a5,a5@(BP_LOWTPA)
	lea	a5@(d7:l),a0
	movl	a0,a5@(BP_HITPA)
	movl	#pgm_start,d0
	movl	d0,a5@(BP_TBASE)/* that's what Pexec(4) will jump to */
	movl	a6@(PRGHDR_TLEN),d1
	movl	d1,a5@(BP_TLEN)
	addl	d1,d0
	movl	d0,a5@(BP_DBASE)
	movl	a6@(PRGHDR_DLEN),d1
	movl	d1,a5@(BP_DLEN)
	addl	d1,d0
	movl	d0,a5@(BP_BBASE)
	movl	d0,a3			/* save for clearing bss */
	movl	a6@(PRGHDR_BLEN),d3
	movl	d3,a5@(BP_BLEN)
	lea	a5@(BP_CMDLIN),a0
	movl	a0,a5@(BP_DTA)
	movl	_sysbase,a0
	movl	a0@(40),a0		/* a0 == p_actpd (p_run) */
	movl	a0@,a0			/* a0 = *p_actpd = actpd */
	movl	a0,a5@(BP_PARENT)	/* put in current process BP */
	movl	#1,a5@(BP_FLAGS)	/* set fastload flag, but not fastram
					 * flags (better use ST-RAM for
					 * DMAread!) */
	lea	a5@(256),a0		/* set up an empty environment in the
					 * space after the basepage (1280
					 * bytes free there) */
	movl	a0,a5@(BP_ENV)
	clrw	a0@
	movl	#-1,a5@(BP_DEVX+0)	/* init standard handles to CON: */
	movw	#-1,a5@(BP_DEVX+4)
	
	/* Clear bss */
	addl	#3,d3
	lsrl	#2,d3
	subql	#1,d3
	movq	#0,d0
1:	movl	d0,a3@+
	dbra	d3,1b
	clrw	d3
	subql	#1,d3
	jbcc	1b

	/* write some stored values to bss */
	movl	a5,__base
	movl	d7,__PgmSize
	movl	#STACK_SIZE,__stksize

	/* Save our current stack contents, because Pexec() uses the same area
	 * as default supervisor stack for new programs. We need to save:
	 * 15 registers (d0-d7/a0-a6), the return addr to the disk boot
	 * routine, 2 local variables there, and the return addr to the main
	 * booting function. All in all 19 longs.
	 */
	movq	#18,d0
	movl	sp,a0
	lea	pc@(stack_save_area),a1
1:	movl	a0@+,a1@+
	dbra	d0,1b
		
/* Output another 'o' to indicate that the Mshrink is done and the basepage
 * has been set up */
	moveq	#'o',d0
	jbsr	putc

	clrl	sp@-
	movl	a5,sp@-
	clrl	sp@-
	movw	#4,sp@-
	movw	#75,sp@-
	trap	#1			/* Pexec(4,NULL,__base,NULL) */
	addw	#16,sp

	/* If the Pexec() returns, the loader process has called Pterm.
	 * Free the memory and decide from _ExitAction what to do.
	 */
	movl	__base,sp@-
	movw	#0x49,sp@-	
	trap	#1        /* Mfree(_base), free the loader's TPA */
	addql	#6,sp

#ifdef DEBUG_EXCEPTIONS
	/* restore old vectors */
	lea	0x8,a1
	lea	save_vecs,a0
	movq	#13,d0
1:	movl	a0@+,a1@+
	dbra	d0,1b
#endif

	/* restore our stack that was probably trashed by Pexec() */
	movq	#18,d0
	movl	sp,a0
	lea	pc@(stack_save_area),a1
1:	movl	a1@+,a0@+
	dbra	d0,1b
	
	movl	_ExitAction,d0
	cmpl	#1,d0
	bne	1f
/* exit_action == 1: jump back to ROM by doing rts. Hopyfully the hd
 * driver is installed now. */

	movew	_bootdev,-(sp)
	movew	#14,-(sp)
	trap	#1		/* Dsetdrv */
	addql	#4,sp

	movml	sp@+,d0-d7/a0-a6 /* pop registers saved by rootsect.S */
	movw	#0xe0,d7 /* for old TOS versions, end boot-try loop (shouldn't
	                  * be necessary, but anyway... */
	rts

1:	cmpl	#2,d0
	bne	1f
/* exit_action == 2: jump to *dskbuf (bootsector) */
	movml	sp@+,d0-d7/a0-a6 /* pop registers saved by rootsect.S */
	/* I don't think we have to set up any register in a special way here,
	 * right? */
	movl	0x4c6,sp@-
	rts

/* default _exit action: print message and halt */
1:	movl	#halt_text,sp@-
	movw	#9,sp@-
	trap	#1			/* Cconws */
	addql	#6,sp	
endless:
	bra	endless
	
		
/*
 * Here's the start of the sub-process. We don't need to Mshrink() our memory
 * (that's already done), and we don't need to change the stack (it's now at
 * the end of the TPA, and that end is the end of our Malloc()ed area, which
 * is fine.)
 */

pgm_start:
/* go to supervisor mode again (Pexec sets sr to 0) */
	clrl	sp@-
	movw	#32,sp@-
	trap	#1
	addql	#6,sp
	
/* Output a 't' to indicate that we're in the subprocess */
	moveq	#'t',d0
	jbsr	putc

	/* Initialize some library variables */
	moveq	#1,d0
	movw	d0,__app
	movw	d0,__split_mem
	clrl	__heapbase
	tstl	___DEFAULT_BUFSIZ__
	jbne	1f
	movl	#BUFSIZ,___DEFAULT_BUFSIZ__
1:	
	jbsr	__init_signal
	
/* Output a 'e' to indicate that we're about to call _main(); there, the final
 * 'd' will be printed */
	moveq	#'e',d0
	jbsr	putc

	/* now jump to _main; simply give it no args and no environment */
	lea	dummy_environ,a0
	movl	a0,_environ
	movl	a0,sp@-
	pea	arg_array
	movl	#1,sp@-
	jbsr	__main
	movl	d0,sp@-
	jbsr	_exit

/* We must supply _exit, otherwise the real crtinit.o from mint.olb is pulled
 * in */
	.globl	__exit
__exit:
	tstl	sp@+			/* pop return address */
	movl	sp@+,d0			/* exit status */
	movw	d0,sp@-			/* push status as 16 bit */
	movw	#76,sp@-
	trap	#1			/* Pterm() */

		
/* Just for debugging: print a character in %d0; no clobbered registers */
putc:
	movml	d0-d2/a0-a2,sp@-
	movw	d0,sp@-
	movw	#2,sp@-
	movw	#3,sp@-
	trap	#13			/* Bconout(CON,c) */
	addql	#6,sp
	movml	sp@+,d0-d2/a0-a2
	rts

#ifdef DEBUG_EXCEPTIONS
	
/* Print string pointed to by a0 */
puts:
	movml	d0/a0,sp@-
2:	movb	a0@+,d0
	beq	1f
	jbsr	putc
	jbra	2b
1:	movml	sp@+,d0/a0
	rts

/* print number in d0 as hex */
putnum:
	movml	d0-d2,sp@-
	movl	d0,d1
	movq	#7,d2
2:	roll	#4,d1
	moveb	d1,d0
	andb	#0x0f,d0
	addb	#'0',d0
	cmpb	#'9',d0
	bls	1f
	addb	#'a'-'0'-10,d0
1:	jbsr	putc
	dbra	d2,2b
	movml	sp@+,d0-d2
	rts

/* print word in d0 as hex */
putword:
	movml	d0-d2,sp@-
	movw	d0,d1
	movq	#3,d2
2:	rolw	#4,d1
	moveb	d1,d0
	andb	#0x0f,d0
	addb	#'0',d0
	cmpb	#'9',d0
	bls	1f
	addb	#'a'-'0'-10,d0
1:	jbsr	putc
	dbra	d2,2b
	movml	sp@+,d0-d2
	rts

exc_addrerr:
	movml	d0-d7/a0-a6,save_regs
	movl	#str_addrerr,a0
	jbsr	puts
	jbra	1f

exc_buserr:
	movml	d0-d7/a0-a6,save_regs
	movl	#str_buserr,a0
	jbsr	puts
1:	movl	sp@(2),d0
	jbsr	putnum
	movl	#str_fa,a0
	jbsr	puts
	movl	sp@(16),d0
	jbsr	putnum
	movl	#str_ssw,a0
	jbsr	puts
	movw	sp@(10),d0
	jbsr	putword
	movl	#crnl,a0
	jbsr	puts
	jbra	exc_common

exc_general:
	movml	d0-d7/a0-a6,save_regs
	movw	sp@(6),d0
	andw	#0xfff,d0
	movl	#exc_names,a0
	movl	a0@(d0:w),a0
	jbsr	puts
	movl	sp@(2),d0
	jbsr	putnum
	movl	#crnl,a0
	jbsr	puts
	jbra	exc_common

exc_common:
	movl	#str_bp,a0
	jbsr	puts
	movl	_sysbase,a0
	movl	a0@(40),a0
	movl	a0@,d0
	jbsr	putnum
	movl	#str_pstart,a0
	jbsr	puts
	lea	pc@(__start),a0
	movl	a0,d0
	jbsr	putnum
	movl	#str_relpc,a0
	jbsr	puts
	movl	sp@(2),d1
	subl	d0,d1
	movl	d1,d0
	jbsr	putnum
	movl	#str_sp,a0
	jbsr	puts
	movw	sp@(6),d1
	andw	#0xf000,d1
	lsrw	#8,d1
	lsrw	#2,d1
	movl	sp,d0
	movl	#framesize,a0
	addl	a0@(d1:w),d0
	movl	d0,a6
	jbsr	putnum
	movl	#str_usp,a0
	jbsr	puts
	movec	usp,d0
	jbsr	putnum
	movl	#str_sr,a0
	jbsr	puts
	movw	sp@,d0
	jbsr	putword
	
	movl	#str_dregs,a0
	jbsr	puts
	movl	#save_regs,a5
	movq	#7,d7
1:	movl	a5@+,d0
	jbsr	putnum
	movq	#' ',d0
	jbsr	putc
	dbra	d7,1b
	movl	#str_aregs,a0
	jbsr	puts
	movq	#6,d7
1:	movl	a5@+,d0
	jbsr	putnum
	movq	#' ',d0
	jbsr	putc
	dbra	d7,1b

	movl	#str_code,a0
	jbsr	puts
	movl	sp@(2),a0
	movq	#13,d7
1:	movw	a0@+,d0
	jbsr	putword
	movq	#' ',d0
	jbsr	putc
	dbra	d7,1b	
	
	movl	#str_stack,a0
	jbsr	puts
	movq	#7,d7
1:	movl	a6,d0
	subl	#16,d0
	jbsr	putnum
	movl	#str_colon,a0
	jbsr	puts
	movq	#7,d6
2:	movl	a6@+,d0
	jbsr	putnum
	movq	#' ',d0
	jbsr	putc
	dbra	d6,2b
	movl	#crnl,a0
	jbsr	puts
	dbra	d7,1b
	
	jbra	endless
			
	.data
crnl:
	.asciz	"\r\n"
str_buserr:
	.asciz	"\r\nBUS ERROR at PC="
str_addrerr:
	.asciz	"\r\nADDRESS ERROR at PC="
str_ill:
	.asciz	"\r\nILLEGAL INSN at PC="
str_zdiv:
	.asciz	"\r\nDIVISION BY ZERO at PC="
str_chk:
	.asciz	"\r\nCHK EXCEPTION at PC="
str_trapcc:
	.asciz	"\r\nTRAPcc at PC="
str_priv:
	.asciz	"\r\nPRIVILEDGE VIOLATION at PC="
str_trace:
	.asciz	"\r\nTRACE at PC="
str_linef:
	.asciz	"\r\nLINE F at PC="
str_copro:
	.asciz	"\r\nCOPROC PROT ERROR at PC="
str_format:
	.asciz	"\r\nFORMAT ERROR at PC="
str_unint:
	.asciz	"\r\nUNINIT INT at PC="
str_fa:
	.asciz	" FA="
str_ssw:
	.asciz	" SSW="
str_bp:
	.asciz	"BP="
str_pstart:
	.asciz	" _start="
str_relpc:
	.asciz	" relPC="
str_sp:
	.asciz	" SP="
str_usp:
	.asciz	" USP="
str_sr:
	.asciz	" SR="
str_colon:
	.asciz	": "
str_dregs:
	.asciz	"\r\nd0-d7: "
str_aregs:
	.asciz	"\r\na0-a6: "
str_code:
	.asciz	"\r\nCode: "
str_stack:
	.asciz	"\r\nStack:\r\n"
	
framesize:
	/* undefined frames use size 8 */
	.long	8,8,12,12,16,8,8,60
	.long	8,20,32,96,8,8,8,8,8

exc_names:
	.long	crnl
	.long	crnl
	.long	crnl		/* buserr */
	.long	crnl		/* addrerr */
	.long	str_ill
	.long	str_zdiv
	.long	str_chk
	.long	str_trapcc
	.long	str_priv
	.long	str_trace
	.long	crnl		/* line a */
	.long	str_linef
	.long	crnl		/* reserved */
	.long	str_copro
	.long	str_format
	.long	str_unint

save_regs:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

save_vecs:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0
		
#endif /* DEBUG_EXCEPTIONS */
	
/* Local Variables:			*/
/*   tab-width:	8			*/
/*   fill-column: 78			*/
/*   asm-comment-char: ?|		*/
/* End:					*/
