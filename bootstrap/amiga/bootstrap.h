/*
** amiga/bootstrap.h -- This file is part of the Amiga bootloader.
**
** Copyright 1993, 1994 by Hamish Macdonald
**
** Some minor additions by Michael Rausch 1-11-94
** Modified 11-May-94 by Geert Uytterhoeven
**			(Geert.Uytterhoeven@cs.kuleuven.ac.be)
**     - inline Supervisor() call
** Modified 10-Jan-96 by Geert Uytterhoeven
**     - The real Linux/m68k boot code moved to linuxboot.[ch]
** Modified 9-Sep-96 by Geert Uytterhoeven
**     - const library bases
**     - fixed register naming for m68k-cbm-amigados-gcc
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** $Id: bootstrap.h,v 1.7 1997-09-19 09:06:23 geert Exp $
** 
** $Log: bootstrap.h,v $
** Revision 1.7  1997-09-19 09:06:23  geert
** Big bunch of changes by Geert: make things work on Amiga; cosmetic things
**
** Revision 1.6  1997/08/12 15:26:54  rnhodek
** Import of Amiga and newly written Atari lilo sources, with many mods
** to separate out common parts.
**
** Revision 1.5  1997/08/10 19:22:54  rnhodek
** Moved AmigaOS inline funcs to extr header inline-funcs.h; the functions
** can't be compiled under Linux
**
** Revision 1.4  1997/07/16 15:06:15  rnhodek
** Replaced all call to libc functions puts, printf, malloc, ... in common code
** by the capitalized generic function/macros. New generic function ReAlloc, need
** by load_ramdisk.
**
** Revision 1.3  1997/07/16 14:05:01  rnhodek
** Sorted out which headers to use and the like; Amiga bootstrap now compiles.
** Puts and other generic functions now defined in bootstrap.h
**
** Revision 1.2  1997/07/16 12:59:13  rnhodek
** Add definitions for generic output and memory allocation
**
** Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
** Import sources into CVS
**
** 
*/

#ifndef _bootstrap_h
#define _bootstrap_h

#include "linuxboot.h"
#include "inline-funcs.h"

struct MsgPort {
    u_char fill1[15];
    u_char mp_SigBit;
    u_char fill2[18];
};

struct IOStdReq {
    u_char fill1[20];
    struct Device *io_Device;
    u_char fill2[4];
    u_short io_Command;
    u_char io_Flags;
    char io_Error;
    u_long io_Actual;
    u_long io_Length;
    void *io_Data;
    u_char fill4[4];
};

#define IOF_QUICK	(1<<0)

struct timerequest {
    u_char fill1[28];
    u_short io_Command;
    u_char io_Flags;
    u_char fill2[1];
    u_long tv_secs;
    u_long tv_micro;
};

#define UNIT_VBLANK	1
#define TR_ADDREQUEST	9


struct Library;
struct IORequest;


static __inline char OpenDevice(u_char *devName, u_long unit,
				struct IORequest *ioRequest, u_long flags)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register u_char *a0 __asm("a0") = devName;
    register u_long d0 __asm("d0") = unit;
    register struct IORequest *a1 __asm("a1") = ioRequest;
    register u_long d1 __asm("d1") = flags;

    __asm __volatile ("jsr a6@(-0x1bc)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (a1), "r" (d0), "r" (d1)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void CloseDevice(struct IORequest *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr a6@(-0x1c2)"
		      : /* no output */
		      : "r" (a6), "r" (a1)
		      : "a0","a1","d0","d1", "memory");
}

static __inline char DoIO(struct IORequest *ioRequest)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr a6@(-0x1c8)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void *CreateIORequest(struct MsgPort *port, u_long size)
{
    register struct Library *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;
    register u_long d0 __asm("d0") = size;

    __asm __volatile ("jsr a6@(-0x28e)"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (d0)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void DeleteIORequest(void *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register void *a0 __asm("a0") = ioRequest;

    __asm __volatile ("jsr a6@(-0x294)"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0","a1","d0","d1", "memory");
}

static __inline struct MsgPort *CreateMsgPort(void)
{
    register struct MsgPort *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;

    __asm __volatile ("jsr a6@(-0x29a)"
		      : "=r" (_res)
		      : "r" (a6)
		      : "a0","a1","d0","d1", "memory");
    return(_res);
}

static __inline void DeleteMsgPort(struct MsgPort *port)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;

    __asm __volatile ("jsr a6@(-0x2a0)"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0","a1","d0","d1", "memory");
}

/* generic output and memory allocation */

#include <stdio.h>

#define Puts(str)		(fputs((str),stderr), fflush(stderr))
#define	GetChar			getchar
#define	PutChar(c)		fputc((c),stderr)
#define	Printf(fmt,rest...)	(fprintf(stderr,fmt,##rest), fflush(stderr))
#define Alloc(size)		AllocVec((size),MEMF_FAST|MEMF_PUBLIC)
#define Free(p)			FreeVec(p)
#define	ReAlloc(ptr,oldsize,newsize)				\
	({												\
		void *__ptr = (ptr);						\
		void *__newptr;								\
		size_t __oldsize = (oldsize);				\
		size_t __newsize = (newsize);				\
													\
		if ((__newptr = Alloc( __newsize ))) {		\
			memcpy( __newptr, __ptr, __oldsize );	\
			Free( __ptr );							\
		}											\
		__newptr;									\
	})
		
	 

#endif  /* _bootstrap_h */

