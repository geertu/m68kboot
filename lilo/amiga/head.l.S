/*
 *  Amiga Linux/m68k Loader -- In-memory AmigaOS Load Module Loader/Executor
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.	See the file COPYING for more details.
 * 
 * $Id: head.l.S,v 1.1 1997-08-12 15:27:03 rnhodek Exp $
 * 
 * $Log: head.l.S,v $
 * Revision 1.1  1997-08-12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <linux/linkage.h>

#include "bootlib.h"


    /*
     *	Alert Codes
     */

AT_DeadEnd		= 0x80000000
AN_LILO			= AT_DeadEnd+0x4c000000 | Lilo Alert Base CC000000
AG_NoMemory		= 0x00010000
AG_BadParm              = 0x00080000
NOT_ENOUGH_MEMORY	= AN_LILO+AG_NoMemory	| Yields Alert Code CC010000
HUNK_ERROR		= AN_LILO+AG_BadParm	| Yields Alert Code CC080000

    /*
     *	Memory Allocation
     */

MEMF_PUBLIC		= 1
MEMF_CLEAR		= 0x10000
MEMF_PUB_CLR		= MEMF_PUBLIC+MEMF_CLEAR

    /*
     *	Exec Library Vector Offsets
     */

LVOAlert		= -0x6c
LVOAllocVec		= -0x2ac
LVOFreeVec		= -0x2b2

    /*
     *  DOS Hunks
     */

HUNK_CODE		= 1001
HUNK_DATA		= 1002
HUNK_BSS		= 1003
HUNK_RELOC32		= 1004
HUNK_END		= 1010
HUNK_HEADER		= 1011


.text

    /*
     *	Header for an AmigaOS Load Module
     *
     *	    in:	    d0 = LiloTable
     *
     *	    out:    d0 = result
     */

ENTRY(_HeadStart)
		moveml	%d2-%d5/%a2-%a6,%sp@-
		movel	%d0,%a5			| LiloTable
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
		movel	%a5@(Lilo_Exec),%a1
		jsr	%a1@
		movel	%d0,%d3			| Result

    /*
     *  Unload the AmigaOS Executable
     */

		movel	%a2,%a3			| Segment Table
		movel	%a2@+,%d2
1:		movel	%a2@+,%a1
		jbsr	%a6@(LVOFreeVec)
		subql	#1,%d2
		jne	1b
		movel	%a3,%a1
		jbsr	%a6@(LVOFreeVec)

		movel	%d3,%d0
		moveml	%sp@+,%d2-%d5/%a2-%a6
		rts


HunkError:	movel	#HUNK_ERROR,%d7
		jmp	%a6@(LVOAlert)

Malloc4:	asll	#2,%d0
		movel	#MEMF_PUB_CLR,%d1
		jsr	%a6@(LVOAllocVec)
		tstl	%d0
		jeq	NoMemory
		rts

NoMemory:	movel	#NOT_ENOUGH_MEMORY,%d7
		jmp	%a6@(LVOAlert)


ENTRY(_HeadEnd)


    /*
     *  Start of Load Module Data
     */

LoadModuleData:

