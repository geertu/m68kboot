/*
 * linuxboot.h -- Header file for linuxboot.c
 *
 * Copyright (c) 1993-97 by
 *   Arjan Knor
 *   Robert de Vries
 *   Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *   Andreas Schwab <schwab@issan.informatik.uni-dortmund.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: linuxboot.h,v 1.9 2004-08-23 16:31:02 joy Exp $
 * 
 * $Log: linuxboot.h,v $
 * Revision 1.9  2004-08-23 16:31:02  joy
 * with this patch ramdisk is loaded to TT/FastRAM even if kernel is in ST-RAM (unless user asked specifically for ramdisk in ST-RAM with new '-R' option in the bootargs). This fixes (or rather works around) problems with ST-RAM swap in kernels 2.4.x. It even helps booting on machines with less RAM. And it also protects the kernel from overwriting by ramdisk. Another switch '-V' additionally protects the Shifter/VIDEL VideoRAM from overwriting by ramdisk
 *
 * Revision 1.8  2004/08/15 11:49:00  geert
 * Add missing Centurbo2 support that I found in atari-bootstrap package
 * (from Petr Stehlik)
 *
 * Revision 1.7  2004/08/15 11:47:06  geert
 * - updates header #include's for compiling with kernel includes 2.2.25
 * - updates Makefile to compile with new cross compiler
 * - removes superfluous declaration of sync()
 * (from Petr Stehlik)
 *
 * Revision 1.6  1998/07/15 08:19:41  schwab
 * Include <asm/types.h> instead of defining __u32 etc types explicitly.
 *
 * Revision 1.5  1998/04/07 09:36:58  rnhodek
 * Add definition of __u32, which is used in 2.1.90+ zorro.h
 *
 * Revision 1.4  1998/02/19 19:44:10  rnhodek
 * Integrated changes from ataboot 3.0 to 3.2
 *
 * Revision 1.3  1997/07/18 12:10:38  rnhodek
 * Call open_ramdisk only if ramdisk_name set; 0 return value means error.
 * Rename load_ramdisk/move_ramdisk to open_ramdisk/load_ramdisk, in parallel
 * to the *_kernel functions.
 * Rewrite open/load_ramdisk so that the temp storage and additional memcpy
 * are avoided if file size known after sopen().
 *
 * Revision 1.2  1997/07/16 09:10:37  rnhodek
 * Made compat_create_machspec_bootinfo return void
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */

#ifndef _linuxboot_h
#define _linuxboot_h

#include <sys/types.h>
#define __KERNEL__
#include <asm/types.h>		/* for 2.1.90+ zorro.h */
#include <asm/bootinfo.h>
#include <asm/setup.h>

/* _MCH cookie values */
#define MACH_ST  	0
#define MACH_STE 	1
#define MACH_TT  	2
#define MACH_FALCON 3

/* some constants for memory handling */
#define TT_RAM_BASE    (u_long)(0x01000000)
#define CT2_FAST_START (u_long)(0x04000000)
#define MB             (1024 * 1024)

/* global variables for communicating options */
extern int debugflag;
extern int ignore_ttram;
extern int load_to_stram;
extern int ramdisk_to_stram;
extern int ramdisk_below_videoram;
extern int force_st_size;
extern int force_tt_size;
extern unsigned long extramem_start;
extern unsigned long extramem_size;
extern char *kernel_name;
extern char *ramdisk_name;
extern char command_line[];
#ifdef USE_BOOTP
/* defined in bootp_mod.c, not linuxboot.c */
extern int no_bootp;
#endif

/* Bootinfo */
struct atari_bootinfo {
    unsigned long machtype;		  /* machine type */
    unsigned long cputype;		  /* system CPU */
    unsigned long fputype;		  /* system FPU */
    unsigned long mmutype;		  /* system MMU */
    int num_memory;			  /* # of memory blocks found */
    struct mem_info memory[NUM_MEMINFO];  /* memory description */
    struct mem_info ramdisk;		  /* ramdisk description */
    char command_line[CL_SIZE];		  /* kernel command line parameters */
    unsigned long mch_cookie;		  /* _MCH cookie from TOS */
    unsigned long mch_type;		  /* special machine types */
};

extern struct atari_bootinfo bi;
extern unsigned long bi_size;
#define MAX_BI_SIZE     (4096)
union _bi_union {
	struct bi_record record;
    u_char fake[MAX_BI_SIZE];
};
extern union _bi_union bi_union;
#ifdef BOOTINFO_COMPAT_1_0
extern struct compat_bootinfo compat_bootinfo;
#endif /* BOOTINFO_COMPAT_1_0 */


/***************************** Prototypes *****************************/

void linux_boot( void ) __attribute__ ((noreturn));
int create_machspec_bootinfo( void);
#ifdef BOOTINFO_COMPAT_1_0
void compat_create_machspec_bootinfo( void);
#endif /* BOOTINFO_COMPAT_1_0 */

/************************* End of Prototypes **************************/


#endif  /* _linuxboot_h */

