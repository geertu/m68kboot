/*
 *  bootstrap/amiga/inline-funcs.h -- Some AmigaOS Inline Functions
 *
 *	Created 1996 by Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 *  
 *  $Id: inline-funcs.h,v 1.3 1998-04-06 01:40:54 dorchain Exp $
 * 
 *  $Log: inline-funcs.h,v $
 *  Revision 1.3  1998-04-06 01:40:54  dorchain
 *  make loader linux-elf.
 *  made amiga bootblock working again
 *  compiled, but not tested bootstrap
 *  loader breaks with MapOffset problem. Stack overflow?
 *
 *  Revision 1.2  1997/08/12 15:26:55  rnhodek
 *  Import of Amiga and newly written Atari lilo sources, with many mods
 *  to separate out common parts.
 *
 */

#ifndef _inline_funcs_h
#define _inline_funcs_h

#include "linuxboot.h"

    /*
     *	Amiga Shared Library/Device Functions
     */

extern const struct ExecBase *SysBase;

#define LVOAllocMem		(-0xc6)
#define LVOAllocVec		(-0x2ac)
#define LVOCacheControl		(-0x288)
#define LVODisable		(-0x78)
#define LVOEnable		(-0x7e)
#define LVOFindResident		(-0x60)
#define LVOFreeMem		(-0xd2)
#define LVOFreeVec		(-0x2b2)
#define LVOOpenresource		(-0x1f2)
#define LVOSuperState		(-0x96)
#define LVOUserState		(-0x9c)
#define LVOSupervisor		(-0x1e)

static __inline void *AllocMem(u_long byteSize, u_long requirements)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = byteSize;
    register u_long d1 __asm("d1") = requirements;

    __asm __volatile ("jsr %%a6@(-0xc6)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void *AllocVec(u_long byteSize, u_long requirements)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = byteSize;
    register u_long d1 __asm("d1") = requirements;

    __asm __volatile ("jsr %%a6@(-0x2ac)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline u_long CacheControl(u_long cacheBits, u_long cacheMask)
{
    register u_long _res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = cacheBits;
    register u_long d1 __asm("d1") = cacheMask;

    __asm __volatile ("jsr %%a6@(-0x288)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void Disable(void)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr %%a6@(-0x78)"
		      : /* no output */
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void Enable(void)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr %%a6@(-0x7e)"
		      : /* no output */
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline struct Resident *FindResident(const u_char *name)
{
    register struct Resident *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register const u_char *a1 __asm("a1") = name;

    __asm __volatile ("jsr %%a6@(-0x60)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void FreeMem(void *memoryBlock, u_long byteSize)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register void *a1 __asm("a1") = memoryBlock;
    register u_long d0 __asm("d0") = byteSize;

    __asm __volatile ("jsr %%a6@(-0xd2)"
		      : /* no output */
		      : "r" (_base), "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void FreeVec(void *memoryBlock)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register void *a1 __asm("a1") = memoryBlock;

    __asm __volatile ("jsr %%a6@(-0x2b2)"
		      : /* no output */
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void *OpenResource(const u_char *resName)
{
    register void *_res  __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register const u_char *a1 __asm("a1") = resName;

    __asm __volatile ("jsr %%a6@(-0x1f2)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void *SuperState(void)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr %%a6@(-0x96)"
		      : "=r" (_res)
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void UserState(void *systack)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register void *d0 __asm("d0") = systack;
    __asm __volatile ("jsr %%a6@(-0x9c)"
    		      : /* no output */
    		      : "r" (_base), "r" (d0)
    		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline u_long Supervisor(u_long (*userfunc)(void))
{
    register u_long _res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long (*d7)() __asm("d7") = userfunc;

    __asm __volatile ("exg %%d7,%%a5;"
		      "jsr %%a6@(-0x1e);"
		      "exg %%d7,%%a5"
		      : "=r" (_res)
		      : "r" (_base), "r" (d7)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct ExpansionBase *ExpansionBase;

#define LVOFindConfigDev	(-0x48)

static __inline struct ConfigDev *FindConfigDev(struct ConfigDev *oldConfigDev,
						long manufacturer, long product)
{
    register struct ConfigDev *_res __asm("d0");
    register const struct ExpansionBase *_base __asm("a6") = ExpansionBase;
    register struct ConfigDev *a0 __asm("a0") = oldConfigDev;
    register long d0 __asm("d0") = manufacturer;
    register long d1 __asm("d1") = product;

    __asm __volatile ("jsr %%a6@(-0x48)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a0), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct GfxBase *GfxBase;
struct View;

#define LVOLoadView		(-0xde)
#define LVOSetChipRev		(-0x378)

static __inline void LoadView(struct View *view)
{
    register const struct GfxBase *_base __asm("a6") = GfxBase;
    register struct View *a1 __asm("a1") = view;

    __asm __volatile ("jsr %%a6@(-0xde)"
		      : /* no output */
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline u_long SetChipRev(u_long want)
{
    register u_long _res __asm("d0");
    register const struct GfxBase *_base __asm("a6") = GfxBase;
    register u_long d0 __asm("d0") = want;

    __asm __volatile ("jsr %%a6@(-0x378)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


    /*
     *	Bootstrap Support Functions
     */

static __inline void disable_mmu(void)
{
    switch(bi.mmutype) {
    	case MMU_68040:
    	case MMU_68060:
	__asm __volatile (".chip 68040;"
			  "moveq #0,%%d0;"
			  ".long 0x4e7b0003;"	/* movec d0,tc */
			  ".long 0x4e7b0004;"	/* movec d0,itt0 */
			  ".long 0x4e7b0005;"	/* movec d0,itt1 */
			  ".long 0x4e7b0006;"	/* movec d0,dtt0 */
			  ".long 0x4e7b0007;"	/* movec d0,dtt1 */
			  ".chip 68k"
			  : /* no outputs */
			  : /* no inputs */
			  : "d0");
		break;
	case MMU_68030:
	case MMU_68851:
	__asm __volatile (".chip 68030;"
			  "subl #4,%sp;"
			  "pmove %tc,%sp@;"
			  "bclr #7,%sp@;"
			  "pmove %sp@,%tc;"
			  "addl #4,%sp;"
			  ".chip 68k");
		if (bi.mmutype == MMU_68030)
	    __asm __volatile ("clrl %sp@-;"
			      ".long 0xf0170800;"	/* pmove sp@,tt0 */
			      ".long 0xf0170c00;"	/* pmove sp@,tt1 */
			      "addql #4,%sp");
		break;
	default:
#if 0
	/* FIXME */
		Printf("Don't know how to disable this MMU!\n");
#endif
		break;
    }
}

#endif
