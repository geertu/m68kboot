/*
 *  linux/arch/m68k/boot/amiga/linuxboot.h -- Generic routine to boot Linux/m68k
 *					      on Amiga, used by both Amiboot and
 *					      Amiga-Lilo.
 *
 *	Created 1996 by Geert Uytterhoeven
 *
 *
 *  This file is based on the original bootstrap code (bootstrap.c):
 *
 *	Copyright (C) 1993, 1994 Hamish Macdonald
 *				 Greg Harp
 *
 *		    with work by Michael Rausch
 *				 Geert Uytterhoeven
 *				 Frank Neumann
 *				 Andreas Schwab
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 * 
 * $Id: linuxboot.h,v 1.10 2000-06-15 18:39:31 dorchain Exp $
 * 
 * $Log: linuxboot.h,v $
 * Revision 1.10  2000-06-15 18:39:31  dorchain
 * Checksumm inline asm was overoptimized by gcc
 *
 * Revision 1.9  2000/06/04 17:14:55  dorchain
 * Fixed compile errors.
 * it still doesn't work for me
 *
 * Revision 1.8  1998/04/07 09:46:46  rnhodek
 * Add definition of __u32, which is used in 2.1.90+ zorro.h
 *
 * Revision 1.7  1997/09/19 09:06:42  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.6  1997/08/12 15:26:55  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * Revision 1.5  1997/08/10 19:22:57  rnhodek
 * Moved AmigaOS inline funcs to extr header inline-funcs.h; the functions
 * can't be compiled under Linux
 *
 * Revision 1.4  1997/07/17 14:18:55  geert
 * Integrate amiboot 5.6 changes (compr. ramdisk and 2.0 kernel)
 *
 * Revision 1.3  1997/07/16 14:05:08  rnhodek
 * Sorted out which headers to use and the like; Amiga bootstrap now compiles.
 * Puts and other generic functions now defined in bootstrap.h
 *
 * Revision 1.2  1997/07/16 09:11:01  rnhodek
 * Made compat_create_machspec_bootinfo return void
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */

#ifndef _linuxboot_h
#define _linuxboot_h

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef unsigned int u32;
typedef unsigned int __u32;

#include <sys/types.h>
#define _LINUX_TYPES_H		/* Hack to prevent including <linux/types.h> */
#define __KERNEL__		/* To get NUM_MEMINFO in setup.h */
#include <asm/bootinfo.h>
#include <asm/setup.h>
#include <linux/zorro.h>
#include "version.h"


    /*
     *  Amiboot Version
     */

#define AMIBOOT_VERSION		VERSION /* Make this the same as main version */


    /*
     *  Amiga Bootinfo Definitions
     *
     *  All limits herein are `soft' limits, i.e. they don't put constraints
     *  on the actual parameters in the kernel.
     */

struct amiga_bootinfo {
    u_long machtype;			/* machine type = MACH_AMIGA */
    u_long cputype;			/* system CPU */
    u_long fputype;			/* system FPU */
    u_long mmutype;			/* system MMU */
    int num_memory;			/* # of memory blocks found */
    struct mem_info memory[NUM_MEMINFO];/* memory description */
    struct mem_info ramdisk;		/* ramdisk description */
    char command_line[CL_SIZE];		/* kernel command line parameters */
    u_long model;			/* Amiga Model */
    int num_autocon;			/* # of autoconfig devices found */
    struct ConfigDev autocon[ZORRO_NUM_AUTO];	/* autoconfig devices */
    u_long chip_size;			/* size of chip memory (bytes) */
    u_char vblank;			/* VBLANK frequency */
    u_char psfreq;			/* power supply frequency */
    u_long eclock;			/* EClock frequency */
    u_long chipset;			/* native chipset present */
    u_short serper;			/* serial port period */
};

int create_machspec_bootinfo(void);
#ifdef BOOTINFO_COMPAT_1_0
void compat_create_machspec_bootinfo(void);
#endif

/* Bootinfo */
extern struct amiga_bootinfo bi;
extern unsigned long bi_size;
#define MAX_BI_SIZE	(4096)
union _bi_union {
    struct bi_record record;
    u_char fake[MAX_BI_SIZE];
};
extern union _bi_union bi_union;
#ifdef BOOTINFO_COMPAT_1_0
extern struct compat_bootinfo compat_bootinfo;
#endif /* BOOTINFO_COMPAT_1_0 */


    /*
     *  Parameters passed to linuxboot()
     */

struct linuxboot_args {
    struct amiga_bootinfo bi;	/* Initial values override detected values */
    const char *kernelname;
    const char *ramdiskname;
    int debugflag;
    int keep_video;
    int reset_boards;
    u_int baud;
    void (*sleep)(u_long micros);
};


    /*
     *  Boot the Linux/m68k Operating System
     */

extern u_long linuxboot(const struct linuxboot_args *args);


    /*
     *  Amiga Models
     */

extern const char *amiga_models[];
extern const u_long first_amiga_model;
extern const u_long last_amiga_model;


    /*
     *	Exec Library Definitions
     */

#define TRUE	(1)
#define FALSE	(0)


struct List {
    struct Node *lh_Head;
    struct Node *lh_Tail;
    struct Node *lh_TailPred;
    u_char lh_Type;
    u_char l_pad;
};

struct MemChunk {
     struct MemChunk *mc_Next;	/* pointer to next chunk */
     u_long mc_Bytes;		/* chunk byte size    */
};

#define MEMF_PUBLIC	(1<<0)
#define MEMF_CHIP	(1<<1)
#define MEMF_FAST	(1<<2)
#define MEMF_LOCAL	(1<<8)
#define MEMF_CLEAR	(1<<16)

struct MemHeader {
    struct Node mh_Node;
    u_short mh_Attributes;	/* characteristics of this region */
    struct MemChunk *mh_First;	/* first free region */
    void *mh_Lower;		/* lower memory bound */
    void *mh_Upper;		/* upper memory bound+1 */
    u_long mh_Free;		/* total number of free bytes */
};

struct ExecBase {
    u_char fill1[20];
    u_short Version;
    u_char fill2[274];
    u_short AttnFlags;
    u_char fill3[24];
    struct List MemList;
    u_char fill4[194];
    u_char VBlankFrequency;
    u_char PowerSupplyFrequency;
    u_char fill5[36];
    u_long ex_EClockFrequency;
    u_char fill6[60];
};

#define AFB_68020	(1)
#define AFF_68020	(1<<AFB_68020)
#define AFB_68030	(2)
#define AFF_68030	(1<<AFB_68030)
#define AFB_68040	(3)
#define AFF_68040	(1<<AFB_68040)
#define AFB_68881	(4)
#define AFF_68881	(1<<AFB_68881)
#define AFB_68882	(5)
#define AFF_68882	(1<<AFB_68882)
#define AFB_FPU40	(6)		/* ONLY valid if AFB_68040 or AFB_68060 */
#define AFF_FPU40	(1<<AFB_FPU40)	/* is set; also set for 68060 FPU */
#define AFB_68060	(7)
#define AFF_68060	(1<<AFB_68060)

struct Resident;


    /*
     *	Graphics Library Definitions
     */

struct GfxBase {
    u_char fill1[20];
    u_short Version;
    u_char fill2[194];
    u_short NormalDisplayRows;
    u_short NormalDisplayColumns;
    u_char fill3[16];
    u_char ChipRevBits0;
    u_char fill4[307];
};

#define GFXB_HR_AGNUS	(0)
#define GFXF_HR_AGNUS	(1<<GFXB_HR_AGNUS)
#define GFXB_HR_DENISE	(1)
#define GFXF_HR_DENISE	(1<<GFXB_HR_DENISE)
#define GFXB_AA_ALICE	(2)
#define GFXF_AA_ALICE	(1<<GFXB_AA_ALICE)
#define GFXB_AA_LISA	(3)
#define GFXF_AA_LISA	(1<<GFXB_AA_LISA)

    /*
     *	HiRes(=Big) Agnus present; i.e. 
     *	1MB chipmem, big blits (none of interest so far) and programmable sync
     */
#define GFXG_OCS	(GFXF_HR_AGNUS)
    /*
     *	HiRes Agnus/Denise present; we are running on ECS
     */
#define GFXG_ECS	(GFXF_HR_AGNUS|GFXF_HR_DENISE)
    /*
     *	Alice and Lisa present; we are running on AGA
     */
#define GFXG_AGA	(GFXF_AA_ALICE|GFXF_AA_LISA)

#define SETCHIPREV_BEST	(0xffffffff)
#define HIRES		(0x8000)


#endif  /* _linuxboot_h */

