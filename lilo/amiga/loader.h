/*
 *  Amiga Linux/m68k Loader -- Loader Definitions
 *
 *  � Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: loader.h,v 1.2 1997-09-19 09:06:54 geert Exp $
 * 
 * $Log: loader.h,v $
 * Revision 1.2  1997-09-19 09:06:54  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:05  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _loader_h
#define _loader_h

#include "loader_common.h"
#include <asm/amigahw.h>
#include "amigaos.h"


    /*
     *  m68k-cbm-amigados-gcc uses a.out Linkage Conventions
     */

#undef SYMBOL_NAME_STR
#define SYMBOL_NAME_STR(X) "_"#X


    /*
     *  Library Versions
     */

#define V36_COMPATIBILITY	/* Make it work with the A3000 Beta ROM */

#ifdef V36_COMPATIBILITY
#define LIB_VERS		(36)
#else /* V36_COMPATIBILITY */
#define LIB_VERS		(37)
#endif /* V36_COMPATIBILITY */


    /*
     *  Custom Hardware Bases
     *
     *  Make sure these are the physical and not the virtual addresses
     */

#undef custom
#define custom		((*(volatile struct CUSTOM *)(CUSTOM_PHYSADDR)))
#undef ciaa
#define ciaa		((*(volatile struct CIA *)(CIAA_PHYSADDR)))


    /*
     *  Lilo Alert
     */

#define AN_LILO		(AT_DeadEnd|0x4c000000) /* `L' */
#define AO_LiloMap	(0x004d4150)            /* `MAP': Corrupt Map File */


    /*
     *  Boot Parameters
     */

#define PATH_SIZE	256		/* Should be sufficient */

struct BootData {
    u_long OSType;
    char Kernel[PATH_SIZE];
    char Ramdisk[PATH_SIZE];
    char Args[CL_SIZE];
    u_long Debug;
    u_long Model;
    u_long ChipSize;
    u_int NumMemory;
    struct FastChunk {
	u_long Address;
	u_long Size;
    } FastChunks[NUM_MEMINFO];
    u_long Processor;
};


    /*
     *  Function Prototypes
     */

extern struct BootData BootData;

extern const struct BootRecord *FindBootRecord(const char *name);
extern void Puts(const char *str);
extern void PutChar(int c);
extern void Printf(const char *fmt, ...);
extern char *ReadCommandLine(void);
extern void ListRecords(void);
extern void ListFiles(void);
extern void FillBootData(const struct BootRecord *record,
			 struct BootData *boot, int autoboot);
extern int TimedOut(void);
extern void ClearTimeOut(void);
extern void SetTimeOut(u_long seconds);
extern void Monitor(void); 


#endif  /* _loader_h */
