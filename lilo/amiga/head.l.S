/*
 *  Amiga Linux/m68k Loader -- In-memory AmigaOS Load Module Loader/Executor
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.	See the file COPYING for more details.
 * 
 * $Id: head.l.S,v 1.2 1997-09-19 09:06:53 geert Exp $
 * 
 * $Log: head.l.S,v $
 * Revision 1.2  1997-09-19 09:06:53  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <linux/linkage.h>

#include "amigaos.h"


AN_LILO = AT_DeadEnd+0x4c000000


.text

    /*
     *	Header for an AmigaOS Load Module
     *
     *	    in:	    d0 = BootRequest
     *
     *	    out:    d0 = result
     */

ENTRY(_HeadStart)
		moveml	%d2-%d5/%a2-%a6,%sp@-
		movel	%d0,%a5			| BootRequest
		movel	4:w,%a6			| SysBase

    /*
     *  Load the AmigaOS Executable
     */

		lea	%pc@(LoadModuleData),%a4
		cmpl	#HUNK_HEADER,%a4@+
		jne	HunkError
		tstl	%a4@+
		jne	HunkError

    /*
     *  Allocate the Segment Table
     */

		movel	%a4@+,%d0
		movel	%d0,%d5			| Number of Hunks
		addql	#1,%d0
		jbsr	Malloc4
		movel	%d0,%a2
		movel	%d5,%a2@
		tstl	%a4@+			| First Hunk = 0
		jne	HunkError
		movel	%a4@+,%d0		| Last Hunk = Number of Hunks-1
		addql	#1,%d0
		cmpl	%d0,%d5
		jne	HunkError

    /*
     *  Allocate the Hunks in the Segment Table
     */

		moveq	#0,%d2
1:		movel	%a4@+,%d0
		jbsr	Malloc4
		movel	%d0,%a2@(4,%d2:l:4)
		addql	#1,%d2
		cmpl	%d2,%d5
		jne	1b

    /*
     *  Process the Hunks
     */

		moveq	#0,%d2
NextHunk:	movel	%a2@(4,%d2:l:4),%a3
		movel	%a4@+,%d0
		subl	#HUNK_END,%d0
		jeq	HunkEnd
		addl	#HUNK_END-HUNK_DATA,%d0
		jeq	HunkCodeData
		addl	#HUNK_DATA-HUNK_CODE,%d0
		jeq	HunkCodeData
		subl	#HUNK_BSS-HUNK_CODE,%d0
		jeq	HunkBss
		subl	#HUNK_RELOC32-HUNK_BSS,%d0
		jne	HunkError

HunkReloc32:	movel	%a4@+,%d3
		jeq	NextHunk
		movel	%a4@+,%d0
		movel	%a2@(4,%d0:l:4),%d4
1:		movel	%a4@+,%d0
		addl	%d4,%a3@(%d0:l)
		subql	#1,%d3
		jne	1b
		jra	HunkReloc32

HunkCodeData:	movel	%a4@+,%d0
		jeq	NextHunk
1:		movel	%a4@+,%a3@+
		subql	#1,%d0
		jne	1b
		jra	NextHunk

HunkBss:	addql	#4,%a4
		jra	NextHunk

HunkEnd:	addql	#1,%d2
		cmpl	%d2,%d5
		jne	NextHunk

    /*
     *  Execute the AmigaOS Executable
     */

		movel	%a2@(4),%a0		| First Hunk in Segment Table
		movel	%a5,%d0
		jbsr	Exec
		movel	%d0,%d3			| Result

    /*
     *  Unload the AmigaOS Executable
     */

		movel	%a2,%a3			| Segment Table
		movel	%a2@+,%d2
1:		movel	%a2@+,%a1
		jsr	%a6@(LVOFreeVec)
		subql	#1,%d2
		jne	1b
		movel	%a3,%a1
		jsr	%a6@(LVOFreeVec)

		movel	%d3,%d0
		moveml	%sp@+,%d2-%d5/%a2-%a6
		rts


HunkError:	movel	#AN_LILO+AG_BadParm,%d7
		jmp	%a6@(LVOAlert)

Malloc4:	asll	#2,%d0
		movel	#MEMF_PUBLIC+MEMF_CLEAR,%d1
		jsr	%a6@(LVOAllocVec)
		tstl	%d0
		jeq	NoMemory
		rts

NoMemory:	movel	#AN_LILO+AG_NoMemory,%d7
		jmp	%a6@(LVOAlert)


    /*
     *	Execute a Loaded File
     *
     *	    in:	    a0 = data
     *		    d0 = arg
     *
     *	    out:    d0 = result
     */

Exec:		moveml	%d2/%a2/%a5,%sp@-
		movel	%a0,%a2
		movel	%d0,%d2
		lea	%a6@(LVOCacheClearU),%a1
		cmpw	#37,%a6@(LIB_VERSION)
		jcc	Cache37			| Only V37+

		lea	%pc@(Cache36),%a1	| Home-made Cache Flush

Cache37:	jsr	%a1@			| Do the Flush
		movel	%d2,%d0			| Pass Args
		jsr	%a2@			| Call Loaded Program
		moveml	%sp@+,%d2/%a2/%a5
		rts

Cache36:	lea	%pc@(CacheSV),%a5
		jmp	%a6@(LVOSuperVisor)

    /*
     *	This code only runs on pre-2.0 (also early A3000 Boot ROMs)
     *	I assume there are still a lot of these (I have one too 8)
     *
     *	No need to check for 040, because this ROM doesn't work at all
     *	on anything but a 030...
     */

CacheSV:	movec	%cacr,%d0
		orw	#0x00000808,%d0		| clear 030 caches
		movec	%d0,%cacr
		rte


ENTRY(_HeadEnd)


    /*
     *  Start of Load Module Data
     */

LoadModuleData:
