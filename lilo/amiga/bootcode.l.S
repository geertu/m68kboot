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
 * $Id: bootcode.l.S,v 1.1 1997-08-12 15:27:00 rnhodek Exp $
 * 
 * $Log: bootcode.l.S,v $
 * Revision 1.1  1997-08-12 15:27:00  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <linux/linkage.h>


    /*
     *	Alert Codes
     */

AT_DeadEnd		= 0x80000000
AN_LILO			= AT_DeadEnd+0x4c000000 | Lilo Alert Base CC000000
AG_NoMemory		= 0x00010000
AG_OpenDev		= 0x00040000
AG_IOError		= 0x00060000
AG_NoSignal		= 0x00070000
NOT_ENOUGH_MEMORY	= AN_LILO+AG_NoMemory	| Yields Alert Code CC010000
DEVICE_OPEN_ERROR	= AN_LILO+AG_OpenDev	| Yields Alert Code CC040000
READ_ERROR		= AN_LILO+AG_IOError	| Yields Alert Code CC060000
NO_MSGPORT		= AN_LILO+AG_NoSignal	| Yields Alert Code CC070000

    /*
     *	Memory Allocation
     */

MEMF_PUBLIC		= 1
MEMF_CLEAR		= 0x10000
MEMF_PUB_CLR		= MEMF_PUBLIC+MEMF_CLEAR

    /*
     *	Exec I/O
     */

IO_COMMAND		= 28
IO_FLAGS		= 30
IO_LENGTH		= 36
IO_DATA			= 40
IO_OFFSET		= 44
CMD_READ		= 2
IOF_QUICK		= 1

    /*
     *	Trackdisk
     */

TD_MOTOR		= 9
IOEXTTD_SIZE		= 56

    /*
     *	Library
     */

LIB_VERSION		= 20

    /*
     *	Resident Tag
     */

RT_INIT			= 22

    /*
     *	Expansion Library
     */

eb_Flags		= 34
EBB_SILENTSTART		= 6

    /*
     *	Exec Library Vector Offsets
     */

LVOSuperVisor		= -0x1e
LVOFindResident		= -0x60
LVOAlert		= -0x6c
LVOCloseLibrary		= -0x19e
LVOOpenDevice		= -0x1bc
LVOCloseDevice		= -0x1c2
LVODoIO			= -0x1c8
LVOOpenLibrary		= -0x228
LVOCopyMem		= -0x270
LVOCacheClearU		= -0x27c		| Only V37+
LVOCreateIORequest	= -0x28e
LVODeleteIORequest	= -0x294
LVOCreateMsgPort	= -0x29a
LVODeleteMsgPort	= -0x2a0
LVOAllocVec		= -0x2ac
LVOFreeVec		= -0x2b2

    /*
     *	Disk Block Size
     */

LILO_BLOCKSIZE		= 1024
LILO_BLOCKSIZESHIFT	= 10

    /*
     *	68030 Cache Flags
     */

FLUSH_I_AND_D		= 0x00000808

    /*
     *	Lilo Data Offsets (Stack Frame)
     */

SysBase			=   -1*4		| ExecBase
BootRequest		=   -2*4		| Passed by Bootstrap
FuncTable		=   -9*4		| Function Table
FileVector		=  -10*4
FileOffset		=  -11*4
FileBuffer		=  -12*4
AltPort			=  -13*4
BASE_SIZE		=   13*4


.text

    /*
     *	Boot Code
     *
     *	    in:	    a1 = bootrequest
     *		    a6 = execbase
     *
     *	    out:    d0 = error
     */

ENTRY(_BootCodeStart)
		moveml	%d2-%d7/%a2-%a6,%sp@-

    /*
     *	Set Up the Stack Frame
     */

		link	%a5,#-BASE_SIZE
		lea	%pc@(BaseVar),%a0
		movel	%a5,%a0@
		movel	%a6,%a5@(SysBase)
		movel	%a1,%a5@(BootRequest)

    /*
     *	Set Up the Function Table
     */

		lea	%a5@(FuncTable),%a0
		lea	%pc@(lilo_open),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_close),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_read),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_seek),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_load),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_unload),%a1
		movel	%a1,%a0@+
		lea	%pc@(lilo_exec),%a1
		movel	%a1,%a0@+

    /*
     *	Allocate the File Buffer
     */

		moveq	#LILO_BLOCKSIZE/16,%d0
		asll	#4,%d0
		jbsr	SafeMalloc
		movel	%d0,%a5@(FileBuffer)

    /*
     *	Alternate Boot Device Initialization
     */

		tstb	%pc@(_AltDeviceName)
		jeq	NoAltDevice
		jsr	%a6@(LVOCreateMsgPort)
		movel	%d0,%a5@(AltPort)
		jne	AltPortOK
		movel	#NO_MSGPORT,%d7
		jra	Alert
AltPortOK:	movel	%d0,%a0
		moveq	#IOEXTTD_SIZE,%d0
		jsr	%a6@(LVOCreateIORequest)
		movel	%d0,%a5@(BootRequest)
		jeq	NoMemory
		movel	%d0,%a1
		lea	%pc@(_AltDeviceName),%a0
		movel	%pc@(_AltDeviceUnit),%d0
		moveq	#0,%d1
		jsr	%a6@(LVOOpenDevice)
		tstl	%d0
		jeq	NoAltDevice
		movel	#DEVICE_OPEN_ERROR,%d7
		jra	Alert

    /*
     *	Load and Execute the Boot Loader
     */

NoAltDevice:	lea	%pc@(_LoaderVectorStart),%a0
		jbsr	lilo_load
		movel	%d0,%a2
		lea	%a5@(FuncTable),%a0
		movel	%a0,%d0
		movel	%a2,%a0
		jbsr	lilo_exec
		movel	%d0,%d2
		movel	%a2,%a0
		jbsr	lilo_unload
		movel	%a5@(FileBuffer),%a1
		jbsr	Free

    /*
     *	Alternate Boot Device Clean Up
     */

		tstb	%pc@(_AltDeviceName)
		jeq	NoAltDevice2
		movel	%a5@(BootRequest),%a1
		jsr	%a6@(LVOCloseDevice)
		movel	%a5@(BootRequest),%a0
		jsr	%a6@(LVODeleteIORequest)
		movel	%a5@(AltPort),%a0
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
BootReturn:	unlk	%a5
		moveml	%sp@+,%d2-%d7/%a2-%a6
		rts


    /*
     *	Open a File
     *
     *	    in:	    a0 = vector
     *
     *	    out:    void
     */

lilo_open:	movel	%pc@(BaseVar),%a1
		movel	%a0,%a1@(FileVector)
		clrl	%a1@(FileOffset)
		rts

    /*
     *	Close the File
     *
     *	    in:	    void
     *
     *	    out:    void
     */

lilo_close:	movel	%a6,%sp@-
		movel	%pc@(BaseVar),%a1
		movel	%a1@(SysBase),%a6
		movel	%a1@(BootRequest),%a1
		movew	#TD_MOTOR,%a1@(IO_COMMAND)
		moveb	#IOF_QUICK,%a1@(IO_FLAGS)
		clrl	%a1@(IO_LENGTH)
		jsr	%a6@(LVODoIO)
		movel	%sp@+,%a6
		rts

    /*
     *	Read from the File
     *
     *	    in:	    a0 = dest
     *		    d0 = length
     *
     *	    out:    void
     */

lilo_read:	moveml	%d2-%d3/%a2/%a5-%a6,%sp@-
		movel	%pc@(BaseVar),%a5
		movel	%a5@(SysBase),%a6
		movel	%d0,%d2
		movel	%a0,%a2
ReadLoop:	tstl	%d2
		jeq	ReadEnd
		movel	%a5@(FileOffset),%d3
		movel	%d3,%d0
		moveq	#LILO_BLOCKSIZESHIFT,%d1
		asrl	%d1,%d0
		movel	%a5@(FileVector),%a0
		movel	%a0@(4,%d0:l:4),%d0
		jeq	Hole
		movel	%a5@(BootRequest),%a1
		movew	#CMD_READ,%a1@(IO_COMMAND)
		moveb	#IOF_QUICK,%a1@(IO_FLAGS)
		movel	#LILO_BLOCKSIZE,%a1@(IO_LENGTH)
		movel	%a5@(FileBuffer),%a1@(IO_DATA)
		movel	%d0,%a1@(IO_OFFSET)
		jsr	%a6@(LVODoIO)
		tstl	%d0
		jeq	SectorReady
		movel	#READ_ERROR,%d7
		jra	Alert
Hole:		movel	#LILO_BLOCKSIZE/4-1,%d1
		movel	%a5@(FileBuffer),%a0
ClearLoop:	clrl	%a0@+
		dbf	%d1,ClearLoop
SectorReady:	moveq	#LILO_BLOCKSIZE/16,%d0
		asll	#4,%d0
		andl	#LILO_BLOCKSIZE-1,%d3
		subl	%d3,%d0
		cmpl	%d2,%d0
		jlt	NotAll
		movel	%d2,%d0
NotAll:		movel	%a5@(FileBuffer),%a0
		lea	%a0@(%d3:l),%a0
		movel	%a2,%a1
		movel	%d0,%d3
		jsr	%a6@(LVOCopyMem)
		subl	%d3,%d2
		addl	%d3,%a2
		addl	%d3,%a5@(FileOffset)
		jra	ReadLoop
ReadEnd:	moveml	%sp@+,%d2-%d3/%a2/%a5-%a6
Back:		rts

    /*
     *	Memory Allocation/Deallocation
     *	(made subroutines to conserve the tight space)
     */

SafeMalloc:	movel	#MEMF_PUB_CLR,%d1
		jsr	%a6@(LVOAllocVec)
		tstl	%d0
		jne	Back

NoMemory:	movel	#NOT_ENOUGH_MEMORY,%d7

Alert:		jmp	%a6@(LVOAlert)

Free:		jmp	%a6@(LVOFreeVec)

    /*
     *	Absolute Seek
     *
     *	    in:	    d0 = offset
     *
     *	    out:    void
     */

lilo_seek:	movel	%pc@(BaseVar),%a1
		movel	%d0,%a1@(FileOffset)
		rts

    /*
     *	Load a File
     *
     *	    in:	    a0 = vector
     *
     *	    out:    d0 = data
     */

lilo_load:	moveml	%d2/%a2/%a6,%sp@-
		movel	%pc@(BaseVar),%a6
		movel	%a6@(SysBase),%a6
		movel	%a0@,%d2
		jbsr	lilo_open
		movel	%d2,%d0
		jbsr	SafeMalloc
		movel	%d0,%a2
		movel	%a2,%a0
		movel	%d2,%d0
		jbsr	lilo_read
		jbsr	lilo_close
		movel	%a2,%d0
		moveml	%sp@+,%d2/%a2/%a6
		rts

    /*
     *	Unload a File
     *
     *	    in:	    a0 = data
     *
     *	    out:    void
     */

lilo_unload:	movel	%a6,%sp@-
		movel	%pc@(BaseVar),%a6
		movel	%a6@(SysBase),%a6
		movel	%a0,%a1
		jbsr	Free
		movel	%sp@+,%a6
		rts


    /*
     *	Execute a Loaded File
     *
     *	    in:	    a0 = data
     *		    d0 = arg
     *
     *	    out:    d0 = result
     */

lilo_exec:	moveml	%d2/%a2/%a5-%a6,%sp@-
		movel	%pc@(BaseVar),%a6
		movel	%a6@(SysBase),%a6
		movel	%a0,%a2
		movel	%d0,%d2
		lea	%a6@(LVOCacheClearU),%a1
		cmpw	#37,%a6@(LIB_VERSION)
		jcc	Cache37			| Only V37+

		lea	%pc@(Cache36),%a1	| Home-made Cache Flush

Cache37:	jsr	%a1@			| Do the Flush
		movel	%d2,%d0			| Pass Args
		jsr	%a2@			| Call Loaded Program
		moveml	%sp@+,%d2/%a2/%a5-%a6
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
		orw	#FLUSH_I_AND_D,%d0	| clear 030 caches
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
		.long	0		| File Size
		.fill	68,4		| Max. 68 Data Blocks
ENTRY(_LoaderVectorEnd)


    /*
     *	Alternate Boot Device
     */

ENTRY(_AltDeviceName)
		.fill	32		| Max. 32 bytes (hardcoded!)

ENTRY(_AltDeviceUnit)
		.long	0


    /*
     *	Base Pointer for Local Variables
     */

BaseVar:	.ascii	"END!"		| Will be overwritten!

ENTRY(_BootCodeEnd)
