/*
 *  Amiga Linux/m68k Loader -- Boot Code
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  Cache fixes for V36 (A3000 Beta ROM) and some code optimizations by Janos
 *  Farkas
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: bootcode.l.S,v 1.3 1998-04-06 01:40:56 dorchain Exp $
 * 
 * $Log: bootcode.l.S,v $
 * Revision 1.3  1998-04-06 01:40:56  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.2  1997/09/19 09:06:50  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:00  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <linux/linkage.h>

#include "amigaos.h"
#include "lilo_util.h"


AN_LILO = AT_DeadEnd+0x4c000000


.text

    /*
     *	Boot Code
     *
     *	    in:	    a1 = bootrequest
     *		    a6 = execbase
     *
     *	    out:    d0 = error
     *
     *
     *  Global variables:
     *
     *	    	    a4 = AltPort
     *		    a5 = BootRequest
     *		    a6 = ExecBase
     */

ENTRY(_BootCodeStart)
		moveml	%d2-%d7/%a2/%a4-%a5,%sp@-
		movel	%a1,%a5

    /*
     *	Alternate Boot Device Initialization
     */

		tstb	%pc@(_AltDeviceName)
		jeq	NoAltDevice
		jsr	%a6@(LVOCreateMsgPort)
		movel	%d0,%a4
		jne	AltPortOK
		movel	#AN_LILO+AG_NoSignal,%d7
		jra	Alert
AltPortOK:	movel	%d0,%a0
		moveq	#IOEXTTD_SIZE,%d0
		jsr	%a6@(LVOCreateIORequest)
		movel	%d0,%a5
		jeq	NoMemory
		movel	%d0,%a1
		lea	%pc@(_AltDeviceName),%a0
		movel	%pc@(_AltDeviceUnit),%d0
		moveq	#0,%d1
		jsr	%a6@(LVOOpenDevice)
		tstl	%d0
		jeq	NoAltDevice
		movel	#AN_LILO+AG_OpenDev,%d7
		jra	Alert

    /*
     *	Load and Execute the Boot Loader
     */

NoAltDevice:	lea	%pc@(_LoaderVectorStart),%a0
		jbsr	Load
		movel	%d0,%a2
		movel	%a5,%d0
		movel	%a2,%a0
		jbsr	Execute
		movel	%d0,%d2
		movel	%a2,%a1
		jbsr	Free

    /*
     *	Alternate Boot Device Clean Up
     */

		tstb	%pc@(_AltDeviceName)
		jeq	NoAltDevice2
		movel	%a5,%a1
		jsr	%a6@(LVOCloseDevice)
		movel	%a5,%a0
		jsr	%a6@(LVODeleteIORequest)
		movel	%a4,%a0
		jsr	%a6@(LVODeleteMsgPort)

NoAltDevice2:	tstl	%d2
		jne	BootAmigaOS
BootError:	moveq	#-1,%d0
		jra	BootReturn

    /*
     *	Boot the Standard Amiga Operating System
     */

BootAmigaOS:	lea	%pc@(ExpansionName),%a1
		moveq	#37,%d0
		jsr	%a6@(LVOOpenLibrary)
		tstl	%d0
		jeq	NoExpansion37
		movel	%d0,%a1
		bset	#EBB_SILENTSTART,%a1@(eb_Flags)
		jsr	%a6@(LVOCloseLibrary)
NoExpansion37:	lea	%pc@(DOSName),%a1
		jsr	%a6@(LVOFindResident)
		tstl	%d0
		jeq	BootError
		movel	%d0,%a0
		movel	%a0@(RT_INIT),%a0
		moveq	#0,%d0
BootReturn:	moveml	%sp@+,%d2-%d7/%a2/%a4-%a5
Back:		rts


    /*
     *	Memory Allocation/Deallocation
     *	(made subroutines to conserve the tight space)
     */

SafeMalloc:	movel	#MEMF_PUBLIC+MEMF_CLEAR,%d1
		jsr	%a6@(LVOAllocVec)
		tstl	%d0
		jne	Back

NoMemory:	movel	#AN_LILO+AG_NoMemory,%d7

Alert:		jmp	%a6@(LVOAlert)

Free:		jmp	%a6@(LVOFreeVec)

    /*
     *	Load a File
     *
     *	    in:	    a0 = vector
     *
     *	    out:    d0 = data
     */

Load:		moveml	%d2-%d3/%a2-%a3,%sp@-
		movel	%a0,%a3
		movel	%a3@+,%d2
		addql	#2,%a3
		movel	#HARD_SECTOR_SIZE-1,%d0
		addl	%d0,%d2
		notl	%d0
		andl	%d0,%d2
		movel	%d2,%d0
		jbsr	SafeMalloc
		movel	%d0,%a2
		movel	%d0,%d3
ReadLoop:	tstl	%d2
		jle	ReadEnd
		moveq	#HARD_SECTOR_SIZE_SHIFT,%d0
		movel	%a3@+,%d1
		asll	%d0,%d1
		movel	%d1,%a5@(IO_OFFSET)
		clrl	%d1
		movew	%a3@+,%d1
		asll	%d0,%d1
		cmpl	%d2,%d1
		jle	1f
		movel	%d2,%d1
1:		movel	%d1,%a5@(IO_LENGTH)
		movel	%a2,%a5@(IO_DATA)
		addl	%d1,%a2
		subl	%d1,%d2
		movew	#CMD_READ,%a5@(IO_COMMAND)
		moveb	#IOF_QUICK,%a5@(IO_FLAGS)
		movel	%a5,%a1
		jsr	%a6@(LVODoIO)
		tstl	%d0
		jeq	ReadLoop
		movel	#AN_LILO+AG_IOError,%d7
		or.w	%d0,%d7
		jra	Alert
ReadEnd:	movew	#TD_MOTOR,%a5@(IO_COMMAND)
		moveb	#IOF_QUICK,%a5@(IO_FLAGS)
		clrl	%a5@(IO_LENGTH)
		movel	%a5,%a1
		jsr	%a6@(LVODoIO)
		movel	%d3,%d0
		moveml	%sp@+,%d2-%d3/%a2-%a3
		rts


    /*
     *	Execute a Loaded File
     *
     *	    in:	    a0 = data
     *		    d0 = arg
     *
     *	    out:    d0 = result
     */

Execute:	moveml	%d2/%a2/%a5,%sp@-
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
     *	No need to check for 68040, because this ROM doesn't work at all
     *	on anything but a 68030...
     */

CacheSV:	movec	%cacr,%d0
		orw	#0x00000808,%d0		| clear 68030 caches
		movec	%d0,%cacr
		rte

    /*
     *	Strings
     */

ExpansionName:	.asciz	"expansion.library"
DOSName:	.asciz	"dos.library"


    /*
     *	File Data for the Loader Program
     */

ENTRY(_LoaderVectorStart)
		.long	0			| File Size
		.short	0			| pad
		.fill	68,6			| Max. 68 Data Blocks
ENTRY(_LoaderVectorEnd)


    /*
     *	Alternate Boot Device
     */

ENTRY(_AltDeviceName)
		.fill	32			| Max. 32 bytes (hardcoded!)

ENTRY(_AltDeviceUnit)
		.long	0


ENTRY(_BootCodeEnd)
