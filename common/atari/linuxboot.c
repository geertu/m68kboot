/*
 * linuxboot.c -- Do actual booting of Linux kernel
 *
 * Copyright (c) 1993-98 by
 *   Arjan Knor
 *   Robert de Vries
 *   Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *   Andreas Schwab <schwab@issan.informatik.uni-dortmund.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: linuxboot.c,v 1.19 2004-08-23 16:31:01 joy Exp $
 * 
 * $Log: linuxboot.c,v $
 * Revision 1.19  2004-08-23 16:31:01  joy
 * with this patch ramdisk is loaded to TT/FastRAM even if kernel is in ST-RAM (unless user asked specifically for ramdisk in ST-RAM with new '-R' option in the bootargs). This fixes (or rather works around) problems with ST-RAM swap in kernels 2.4.x. It even helps booting on machines with less RAM. And it also protects the kernel from overwriting by ramdisk. Another switch '-V' additionally protects the Shifter/VIDEL VideoRAM from overwriting by ramdisk
 *
 * Revision 1.18  2004/08/23 16:16:20  joy
 * displays ARAnyM for _MCH 0x50000 and adds LF to unknown
 *
 * Revision 1.17  2004/08/23 16:15:42  joy
 * corrects ramdisk src address printed in debug mode
 *
 * Revision 1.16  2004/08/23 16:14:16  joy
 * corrects my previous code for finding out the physical address from virtual one
 *
 * Revision 1.15  2004/08/15 12:08:13  geert
 * Allow CT2 and AB40 users to finally load kernel and ramdisk into FastRAM.
 * Previously the bootstrap didn't allow it because it knew the FastRAM was MMU
 * translated. Now it detects the correct offset between virtual and physical
 * addresses and so it works OK (with the exception of the 040 ptestr code that
 * doesn't work correctly until Linux was started at least once - mystery -
 * probably missing a MMU initialization but don't know what exactly).
 * (from Petr Stehlik)
 *
 * Revision 1.14  2004/08/15 11:52:25  geert
 * The bootstrap 3.2/3.3 never printed out how much memory was missing.  This
 * patch corrects it so that users know exactly how much memory they'll need.
 * Sometime the low memory can be fixed simply by removing some programs from
 * memory before starting the bootstrap and this will help users to overcome
 * that.
 * (from Petr Stehlik)
 *
 * Revision 1.13  2004/08/15 11:51:16  geert
 * Correct behavior of "-t" (=ignore TT/FastRAM) so it is truly ignored and no
 * FastRAM test on AB40 is run.
 * (from Petr Stehlik)
 *
 * Revision 1.12  2004/08/15 11:48:59  geert
 * Add missing Centurbo2 support that I found in atari-bootstrap package
 * (from Petr Stehlik)
 *
 * Revision 1.11  2004/08/15 11:47:05  geert
 * - updates header #include's for compiling with kernel includes 2.2.25
 * - updates Makefile to compile with new cross compiler
 * - removes superfluous declaration of sync()
 * (from Petr Stehlik)
 *
 * Revision 1.10  1998/06/12 11:40:03  rnhodek
 * Fix recognition of last 4 MB of memory on AB040.
 *
 * Revision 1.9  1998/02/26 10:05:42  rnhodek
 * Also strip "bootp:" prefixes for BOOT_IMAGE= option.
 * Fix test whether there's enough test to add BOOT_IMAGE=.
 *
 * Revision 1.8  1998/02/25 10:33:29  rnhodek
 * Move call to Super() to bootstrap-specific file bootstrap.c, and
 * remove it from linuxboot.c.
 *
 * Revision 1.7  1998/02/19 19:44:10  rnhodek
 * Integrated changes from ataboot 3.0 to 3.2
 *
 * Revision 1.6  1997/07/18 12:10:38  rnhodek
 * Call open_ramdisk only if ramdisk_name set; 0 return value means error.
 * Rename load_ramdisk/move_ramdisk to open_ramdisk/load_ramdisk, in parallel
 * to the *_kernel functions.
 * Rewrite open/load_ramdisk so that the temp storage and additional memcpy
 * are avoided if file size known after sopen().
 *
 * Revision 1.5  1997/07/16 17:32:02  rnhodek
 * Removed SERROR macro -- unnecessary here. Calls it were wrong.
 *
 * Revision 1.4  1997/07/16 14:05:08  rnhodek
 * Sorted out which headers to use and the like; Amiga bootstrap now compiles.
 * Puts and other generic functions now defined in bootstrap.h
 *
 * Revision 1.3  1997/07/16 11:43:22  rnhodek
 * Removed (already commented-out) inclusion of bootinfo headers
 *
 * Revision 1.2  1997/07/16 09:10:37  rnhodek
 * Made compat_create_machspec_bootinfo return void
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <osbind.h>
#include <sys/file.h>
#include <sys/types.h>

#include <asm/page.h>

#include "linuxboot.h"
#include "loadkernel.h"
#include "bootinf.h"
#include "stream.h"
#include "inline-funcs.h"

#define MIN_RAMSIZE     (3)	/* 3 MB */
#define TEMP_STACKSIZE 256

struct atari_bootinfo bi;
unsigned long bi_size;
union _bi_union bi_union;

#ifdef BOOTINFO_COMPAT_1_0
struct compat_bootinfo compat_bootinfo;
#endif /* BOOTINFO_COMPAT_1_0 */

static u_long *cookiejar;

/* for boot_exit() inline function */
#include "bootstrap.h"

/* global variables for communicating options */
int debugflag = 0;		/* debugging */
int ignore_ttram = 0;		/* ignore TT RAM */
int load_to_stram = 0;		/* put kernel into ST RAM */
int ramdisk_to_stram = 0;	/* put ramdisk into ST RAM */
int ramdisk_below_videoram = 0;	/* don't let ramdisk overwrite videoram */
int force_st_size = -1;		/* force size of ST RAM (-1=autodetect) */
int force_tt_size = -1;		/* force size of TT RAM (-1=autodetect) */
unsigned long extramem_start=0;	/* start of extra memory block (0=none) */
unsigned long extramem_size=0;	/* size of that block */
char *kernel_name = "vmlinux";	/* name of kernel image */
char *ramdisk_name = NULL;	/* name of ramdisk image */
char command_line[CL_SIZE];	/* kernel command line */

unsigned long userstk;

int stram_chunk = 0;		/* index of bi.memory[] ST-RAM block */

/* these are defined in the mover code */
extern char copyall, copyallend;

/* TOS system variables */
#define phystop    ((unsigned long *)0x42e)
#define _p_cookies ((unsigned long **)0x5a0)
#define ramtop     ((unsigned long *)0x5a4)

/***************************** Prototypes *****************************/

static void get_cpu_infos( void );
static void get_mch_type( void );
static void get_mem_infos( void );
static char *format_mb( unsigned long size );
static int getcookie( char *cookie, u_long *value);
static int test_cpu_type (void);
static int test_software_fpu( void);
static void get_medusa_bank_sizes( u_long *bank1, u_long *bank2 );
static int get_ab040_bank_sizes( int maxres, u_long *result );

/************************* End of Prototypes **************************/


#define ERROR(fmt,rest...)			\
    do {					\
	fprintf( stderr, fmt, ##rest );		\
	boot_exit( EXIT_FAILURE );		\
    } while(0)

#define TOS_PAGE_SIZE	8192
#define TOS_PAGE_MASK	(~(TOS_PAGE_SIZE-1))
/* return offset between physical and virtual address */
long phys_offset(u_long vaddr) {
    if (bi.mch_type == ATARI_MACH_AB40 && ((unsigned long)vaddr & 0xff000000)) {
	/* Convert virtual (user) address VADDR to physical address PADDR */
	unsigned long _mmusr, _paddr;
  	vaddr &= TOS_PAGE_MASK;
  __asm__ __volatile__ (".chip 68040\n\t"
			"moveq  #1,d0\n\t"
			"movec  d0,dfc\n\t"
			"ptestr (%1)\n\t"
			"movec %%mmusr,%0\n\t"
			".chip 68k"
			: "=r" (_mmusr)
			: "a" (vaddr));
  	_paddr = _mmusr & TOS_PAGE_MASK;
	return _paddr - vaddr;
    }
    if (getcookie("_CT2", NULL) != -1 && ((unsigned long)vaddr & 0xff000000))
	return CT2_FAST_START - TT_RAM_BASE;
    return 0;
}

void linux_boot( void )
{
    char *kname;
    void *bi_ptr;		/* pointer to bootinfo */
    u_long start_mem, mem_size;	/* addr and size of mem chunk where the kernel
				 * goes to */
    u_long kernel_size;		/* size of kernel image */
    char *memptr;		/* addr and size of load region */
    u_long memreq;
    long memptr_mmu_offset;	/* physical memptr addr offset */
    u_long rd_size;		/* size of ramdisk and array of pointers to
				 * its data */

    /* get the info we need from the cookie-jar */
    cookiejar = *_p_cookies;
    if(cookiejar == 0L)
	/* if we find no cookies, it's probably an ST */
	ERROR( "Error: No cookiejar found. Is this an ST?\n" );

    /* Here we used to warn if MiNT was running, but this proved to be
     * unnecessary */

    /* machine is Atari */
    bi.machtype = MACH_ATARI;

    /* Copy command line options into the kernel command line */
    strcpy( bi.command_line, command_line );
    kname = kernel_name;
    if (strncmp( kernel_name, "local:", 6 ) == 0 ||
	strncmp( kernel_name, "bootp:", 6 ) == 0)
	kname += 6;
    if (strlen(bi.command_line)+strlen(kname)+12 < CL_SIZE-1) {
	if (*bi.command_line)
	    strcat( bi.command_line, " " );
	strcat( bi.command_line, "BOOT_IMAGE=" );
	strcat( bi.command_line, kname );
    }
    printf ("Kernel command line: %s\n", bi.command_line );

    get_cpu_infos();
    get_mch_type();
    
    /* On the Afterburner040, neither bootstrap code nor data may reside in
     * FastRAM. The FastRAM there is joined to one contiguous block with the
     * MMU, and thus FastRAM addresses can become invalid as soon as we turn
     * off the MMU. */
    if (bi.mch_type == ATARI_MACH_AB40 &&
	((unsigned long)linux_boot & 0xff000000))
	ERROR( "Error: Bootstrap can't run in FastRAM on Afterburner040\n" );
    /* For similar reasons, the bootstrap doesn't work in Centurbo2 TT-RAM */
    if (getcookie("_CT2", NULL) != -1 &&
	((unsigned long)linux_boot & 0xff000000))
	ERROR( "Error: Bootstrap can't run in TT-RAM on Centurbo2\n" );
    

    get_mem_infos();
    /* extract data of first chunk, there goes the kernel to */
    /* Load the kernel one page after start of mem */
    start_mem = bi.memory[0].addr + PAGE_SIZE;
    mem_size = bi.memory[0].size - PAGE_SIZE;

#ifdef TEST
    /*
    ** Temporary exit point for testing
    */
    boot_exit(-1);
#endif /* TEST */

    /* load the ramdisk */
    if (ramdisk_name) {
	if (!(rd_size = open_ramdisk( ramdisk_name )))
	    boot_exit( EXIT_FAILURE );
    }
    else
	rd_size = 0;
    bi.ramdisk.size = rd_size;
    
    /* open the kernel image and analyze it */
    if (!(kernel_size = open_kernel( kernel_name )))
	boot_exit( EXIT_FAILURE );

    /* Locate ramdisk in dest. memory */
    if (rd_size) {
	u_long ramdisk_end;
	/* by default put ramdisk into kernel memory block */
	int ramdisk_chunk = 0;
	if (ramdisk_to_stram)
	    ramdisk_chunk = stram_chunk;  /* user wants ramdisk in ST-RAM */
	else {
	    /* if kernel is in ST-RAM then first chunk is not TT-RAM */
	    if (stram_chunk == 0 && bi.num_memory > 1)
		ramdisk_chunk = 1; /* go for the second chunk */
	}
	
	/* if ramdisk is in same memory as kernel */
	if (ramdisk_chunk == 0) {
	    /* make sure they fit */
	    if ( (rd_size + kernel_size) > (bi.memory[0].size - MB/2) )
		ERROR( "Not enough memory for kernel and ramdisk (%ld kB)\n",
		       (rd_size + kernel_size) / 1024 );
	}

	ramdisk_end = (u_long) bi.memory[ramdisk_chunk].addr
		             + bi.memory[ramdisk_chunk].size;

	/* if ramdisk respects videoram and is in ST-RAM */
	if (ramdisk_below_videoram && ramdisk_end < TT_RAM_BASE) {
	    /* make sure it doesn't overlap the videoram */
	    u_long videoram = (u_long)Physbase();
	    if (ramdisk_end > videoram)
		/* put ramdisk below videoram */
		ramdisk_end = videoram;
	}

	bi.ramdisk.addr = ramdisk_end - rd_size;
    }
    
    /* create the bootinfo structure (set bi_size to a value) */
    if (!create_bootinfo())
	ERROR( "Couldn't create bootinfo\n" );

    memreq = kernel_size + bi_size;
#ifdef BOOTINFO_COMPAT_1_0
    if (sizeof(compat_bootinfo) > bi_size)
	memreq = kernel_size+sizeof(compat_bootinfo);
#endif /* BOOTINFO_COMPAT_1_0 */
    /* align load address of ramdisk image, read() is sloooow on odd addr. */
    memreq = ((memreq + 3) & ~3) + rd_size;
    
    /* allocate RAM for the kernel */
    if (!(memptr = malloc( memreq )))
	ERROR( "Unable to allocate %ld kB of memory for kernel%s\n",
		memreq / 1024, (rd_size > 0) ? " and ramdisk" : "" );

    /* clearing the kernel's memory perhaps avoids "uninitialized bss"
     * types of bugs... */
    memset(memptr, 0, memreq - rd_size);

    /* read the text and data segments from the kernel image */
    if (!load_kernel( memptr ))
	boot_exit( EXIT_FAILURE );
    /* load or move ramdisk image to its final resting place */
    if (!load_ramdisk( ramdisk_name, memptr+memreq-rd_size, rd_size ))
	boot_exit( EXIT_FAILURE );

    /* Check kernel's bootinfo version */
    switch (check_bootinfo_version( memptr, MACH_ATARI, ATARI_BOOTI_VERSION,
				    COMPAT_ATARI_BOOTI_VERSION )) {
      case BI_VERSION_MAJOR(ATARI_BOOTI_VERSION):
	bi_ptr = &bi_union.record;
	break;

#ifdef BOOTINFO_COMPAT_1_0
      case BI_VERSION_MAJOR(COMPAT_ATARI_BOOTI_VERSION):
	if (!create_compat_bootinfo())
	    ERROR( "Couldn't create compat bootinfo\n" );
	bi_ptr = &compat_bootinfo;
	bi_size = sizeof(compat_bootinfo);
	break;
#endif /* BOOTINFO_COMPAT_1_0 */

      default:
	ERROR( "Kernel has unsupported bootinfo version\n" );
    }

    /* copy the boot_info struct to the end of the kernel image */
    memcpy( memptr + kernel_size, bi_ptr, bi_size );

    /* find out the physical address if the memory is MMU mapped */
    memptr_mmu_offset = phys_offset((u_long)memptr);

    /* for those who want to debug */
    if (debugflag) {
	if (rd_size) {
	    printf ("ramdisk src at %#lx, size is %ld\n",
		    (u_long)memptr + memreq - rd_size, bi.ramdisk.size);
	    printf ("ramdisk dest is %#lx ... %#lx\n",
		    bi.ramdisk.addr, bi.ramdisk.addr + rd_size - 1 );
	}
	kernel_debug_infos( start_mem );
	printf ("boot_info is at %#lx\n",
		start_mem + kernel_size);

	if (memptr_mmu_offset)
	    printf ("kernel+ramdisk src offset = %#lx\n", memptr_mmu_offset);

	printf ("\nType a key to continue the Linux boot...");
	fflush (stdout);
	getchar();
    }

    /* now here's the point of no return... */
    printf("Booting Linux...\n");
    sync ();

    /* turn off interrupts... */
    disable_interrupts();

    /* turn off caches... */
    disable_cache();

    /* ..and any MMU translation */
    disable_mmu();

    /* correct pointer to physical memory after MMU translation was disabled */
    memptr += memptr_mmu_offset;

    /* ++guenther: allow reset if launched with MiNT */
    *(long*)0x426 = 0;

    /* copy mover code to a safe place if needed */
    memcpy ((void *) 0x400, &copyall, &copyallend - &copyall);

    /* setup stack */
    change_stack ((void *) PAGE_SIZE);

    /*
     * To avoid that the kernel or the ramdisk overwrite the code moving them
     * to there, we first copy the mover code to a safe place.
     * Then this program jumps to the mover code. After the mover code
     * has finished it jumps to the start of the kernel in its new position.
     * I thought the memory just after the interrupt vector table was a safe
     * place because it is used by TOS to store some system variables.
     * This range goes from 0x400 to approx. 0x5B0.
     * This is more than enough for the miniscule mover routine (16 bytes).
     *
     * Some care is also needed to bring both the kernel and the ramdisk into
     * their final resting position without corrupting them (overwriting some
     * data while copying in source or destination areas). It's solved as
     * follows: The source area is a load region, where we have the kernel
     * code and ramdisk data in memory currently. It is important that the
     * ramdisk is above the kernel. The mover then copies the kernel (upwards)
     * to start of memory, and the ramdisk (downwards) to the end of RAM. This
     * ensures none of the copy actions can overwrite the load region. (E.g.,
     * the kernel can't overwrite the ramdisk source, because that is above
     * the kernel source.) If the load region or the ramdisk destination are
     * in another memory block, we don't have any problem anyway.
     */

    jump_to_mover((char *) start_mem, memptr,
		  (char *) bi.ramdisk.addr + rd_size, memptr + memreq,
		  kernel_size + bi_size, rd_size,
		  (void *) 0x400);

    for (;;);
    /* NOTREACHED */
}

static void get_cpu_infos( void )
{
    u_long cpu_type;

    switch( cpu_type = test_cpu_type() ) {
      case  0: ERROR( "Machine type currently not supported. Aborting..." );
      case 20: bi.cputype = CPU_68020; bi.mmutype = MMU_68851; break;
      case 30: bi.cputype = CPU_68030; bi.mmutype = MMU_68030; break;
      case 40: bi.cputype = CPU_68040; bi.mmutype = MMU_68040; break;
      case 60: bi.cputype = CPU_68060; bi.mmutype = MMU_68060; break;
      default:
	ERROR( "Error: Unknown CPU type. Aborting...\n" );
    }
    printf("CPU: %ld; ", cpu_type + 68000);

    printf("FPU: ");
    switch( cpu_type ) {
      case 40: bi.fputype = FPU_68040; puts( "68040" ); break;
      case 60: bi.fputype = FPU_68060; puts( "68060" ); break;
      default:
	if (test_software_fpu()) {
	    puts( "none or software emulation" );
	}
	else {
	    if (fpu_idle_frame_size () != 0x18) {
		bi.fputype = FPU_68882;
	    	puts("68882");
	    }
	    else {
		bi.fputype = FPU_68881;
		puts("68881");
	    }
	}
    }
}

/* Test for Medusa/Hades/Afterburner: These are the machine on which address 0
 * is writeable.
 * Further distinction is made by readability of 0x00ff82fe, which gives a bus
 * error on the Falcon, but not on Medusa/Hades.
 * Medusa and Hades are separated by reading 0xb0000000, a PCI address that
 * exists only on Hades (buserr on Medusa).
 * Return values of the asm are:
 *  0 = standard machine (0x0 not writeable)
 *  1 = failed 0x00ff82fe test -> Afterburner
 *  2 = failed 0xb0000000 test -> Medusa
 *  3 = all tests passed -> Hades
 */

static void get_mch_type( void )
{
    u_long mch_cookie;
    int rv;

    /* Pass contents of the _MCH cookie to the kernel */
    getcookie("_MCH", &mch_cookie);
    bi.mch_cookie = mch_cookie;

    bi.mch_type = ATARI_MACH_NORMAL;
    if (getcookie("_CT2", NULL) != -1)
	/* On the Centurbo2 board, no memory tests will give bus errors! So
	 * don't even execute the test code below, but proceed */
	goto finish;
	

    __asm__ __volatile__
	( "movel	0x8,a0\n\t"	/* save buserr vector */
	  "movel	sp,a1\n\t" 	/* save stack pointer */
	  "movew	sr,d2\n\t"	/* save sr */
	  "orw		#0x700,sr\n\t"	/* disable interrupts */
	  "moveb	0x0,d1\n\t"	/* save old value of 0x0 */
	  "movel	#1f,0x8\n\t"	/* setup new buserr vector */
	  "moveq	#0,%0\n\t"	/* assume no Medusa */
	  "clrb		0x0\n\t"	/* try to write to 0x0 */
	  "nop		\n\t"		/* clear insn pipe */
	  "moveq	#1,%0\n\t"	/* if come here, 0x0 is writeable */
	  "moveb	d1,0x0\n\t"	/* write back saved value */
	  "nop		\n\t"
	  "tstb		0x00ff82fe\n\t"	/* Medusa asserts DTACK here (so no
					 * buserr), but Falcon with AB40 not */
	  "nop		\n\t"
	  "moveq	#2,%0\n\t"	/* if come here, it's a Medusa or
					 * Hades */
	  "nop		\n\t"
	  "tstb		0xb0000000\n\t"	/* PCI address on Hades */
	  "nop		\n\t"
	  "moveq	#3,%0\n"	/* if PCI exists, then no Medusa */
	  "1:\t"
	  "movel	a1,sp\n\t"	/* restore stack pointer */
	  "movel	a0,0x8\n\t"	/* restore buserr vector */
	  "movew	d2,sr"		/* reenable ints */
	  : "=d" (rv)
	  : /* no inputs */
	  : "d1", "d2", "a0", "a1", "memory" );

    switch( rv ) {
      case 1:
	bi.mch_type = ATARI_MACH_AB40;
	break;
      case 2:
	bi.mch_type = ATARI_MACH_MEDUSA;
	break;
      case 3:
	bi.mch_type = ATARI_MACH_HADES;
	break;
    }

    /* Unfortunately, 0x0 isn't writeable on all Afterburners (yes, it seems
     * so...), so the test above for AB40 may fail. But it surely is an AB40
     * if an "AB40" cookie exists, or with 99% if it's a Falcon and has a '040
     * processor. */
    if (bi.mch_type == ATARI_MACH_NORMAL) {
	if (getcookie("AB40", NULL) != -1 ||
	    ((bi.mch_cookie >> 16) == ATARI_MCH_FALCON &&
	     bi.cputype == CPU_68040))
	    bi.mch_type = ATARI_MACH_AB40;
    }

  finish:
    printf( "Model: " );
    switch( bi.mch_cookie >> 16 ) {
      case ATARI_MCH_ST:
	puts( "ST" );
	break;
      case ATARI_MCH_STE:
	if (bi.mch_cookie & 0xffff)
	    puts( "Mega STE" );
	else
	    puts( "STE" );
	break;
      case ATARI_MCH_TT:
	/* Medusa and Hades have TT _MCH cookie */
	if (bi.mch_type == ATARI_MACH_MEDUSA)
	    puts( "Medusa" );
	else if (bi.mch_type == ATARI_MACH_HADES)
	    puts( "Hades" );
	else
	    puts( "TT" );
	break;
      case ATARI_MCH_FALCON:
	printf( "Falcon" );
	if (bi.mch_type == ATARI_MACH_AB40)
	    printf( " (with Afterburner040)" );
	printf( "\n" );
	break;
      case 5:
	puts( "ARAnyM" );
	break;
      default:
	printf( "unknown mach cookie 0x%lx\n", bi.mch_cookie );
	break;
    }
}

#define GRANULARITY (256*1024) /* min unit for memory */
#define ADD_CHUNK(start,siz,force,kindstr)			\
    do {							\
	unsigned long _start = (start);				\
	unsigned long _size  = (siz) & ~(GRANULARITY-1);	\
	int _force = (force);					\
								\
	if (_force >= 0) _size = _force;			\
	if (_size > 0) {					\
	    bi.memory[chunk].addr = _start;			\
	    bi.memory[chunk].size = _size;			\
	    total += _size;					\
	    printf( kindstr ": %s MB at 0x%08lx\n",		\
		    format_mb(_size), _start );			\
	    chunk++;						\
	}							\
    } while(0)

/* Get the amounts of ST- and TT-RAM. */
static void get_mem_infos( void )
{
    int chunk = 0;
    unsigned long total = 0;

    if (force_st_size >= 0 && force_st_size < 256*1024) {
	force_st_size = 256*1024;
	printf( "Need at least 256k ST-RAM! Changing -S to 256k.\n" );
    }
    

    if (bi.mch_type == ATARI_MACH_MEDUSA) {

	/* For the Medusa, some things are different... */
	
        unsigned long bank1, bank2, medusa_st_ram;
	int fake_force;

        get_medusa_bank_sizes( &bank1, &bank2 );
	medusa_st_ram = *phystop & ~(MB - 1);
        bank1 -= medusa_st_ram;

	/* For the Medusa, load_to_stram is ignored, the kernel is always at
	 * 0; both kinds of RAM are the same and equally fast */
	if (load_to_stram)
	    printf( "(Note: -s ignored on Medusa)\n" );
	if (ramdisk_to_stram)
	    printf( "(Note: -R ignored on Medusa)\n" );

	ADD_CHUNK( 0, medusa_st_ram,
		   force_st_size, "Medusa pseudo ST-RAM from bank 1" );

	fake_force = force_tt_size < 0 ? -1 :
		     force_tt_size <= bank1 ? force_tt_size : bank1;
        if (!ignore_ttram && bank1 > 0)
	    ADD_CHUNK( 0x20000000 + medusa_st_ram, bank1,
		       fake_force, "TT-RAM bank 1" );
	fake_force = force_tt_size < 0 ? -1 :
		     force_tt_size <= bank1 ? 0 : force_tt_size - bank1;
        if (!ignore_ttram && bank2 > 0)
	    ADD_CHUNK( 0x24000000, bank2,
		       fake_force, "TT-RAM bank 2" );
			
        bi.num_memory = chunk;
    }
    else if (bi.mch_type == ATARI_MACH_AB40) {

	/* Assume that only Falcon with a '040 is Afterburner040 */

	if (!ignore_ttram) {
            struct {
	        unsigned long start, size;
	    } banks[2];
	    int n_banks;

            n_banks = get_ab040_bank_sizes( 2, (unsigned long *)banks );
	
            if (n_banks >= 1)
	        ADD_CHUNK( banks[0].start, banks[0].size,
		           force_tt_size, "FastRAM bank 1" );
            if (n_banks >= 2 && force_tt_size < 0)
	        ADD_CHUNK( banks[1].start, banks[1].size,
		           force_tt_size, "FastRAM bank 2" );
	}

    }
    else {

	/* TT RAM tests for standard machines */
	struct {
		unsigned short version; /* version - currently 1 */
		unsigned long fr_start; /* start addr FastRAM */
		unsigned long fr_len;   /* length FastRAM */
	} *magn_cookie;
	struct {
		unsigned long version;
		unsigned long fr_start; /* start addr */
		unsigned long fr_len;   /* length */
	} *fx_cookie;

        if (!ignore_ttram) {
	    /* all these kinds of TT-RAM are mutually exclusive */

	    /* "Original" or properly emulated TT-Ram */
	    if (*ramtop) {
		/* the 'ramtop' variable at 0x05a4 is not
		 * officially documented. We use it anyway
		 * because it is the only way to get the TTram size.
		 * (It is zero if there is no TTram.)
		 */
		if (getcookie("_CT2", NULL) != -1)
		    /* TT-RAM starts at phys. 0x04000000 on Centurbo2 but is
		     * remapped for TOS to 0x01000000 with the MMU */
		    ADD_CHUNK( CT2_FAST_START, *ramtop - TT_RAM_BASE,
			       force_tt_size, "TT-RAM" );
		else
		    ADD_CHUNK( TT_RAM_BASE, *ramtop - TT_RAM_BASE,
			       force_tt_size, "TT-RAM" );
	    }
	    /* test for MAGNUM alternate RAM
	     * added 26.9.1995 M. Schwingen, rincewind@discworld.oche.de
	     */
	    else if (getcookie("MAGN", (u_long *)&magn_cookie) != -1)
		ADD_CHUNK( magn_cookie->fr_start,
			   magn_cookie->fr_len,
			   force_tt_size, "MAGNUM alternate RAM" );
	    /* BlowUps FX */
	    else if (getcookie("BPFX", (u_long *)&fx_cookie) != -1 &&
		     fx_cookie)
		/* if fx is set (cookie call above),
		 * we assume that BlowUps FX-card
		 * is installed. (Nat!)
		 */
		ADD_CHUNK( fx_cookie->fr_start,
			   fx_cookie->fr_len,
			   force_tt_size, "FX alternate RAM" );

	}
    }

    if (bi.mch_type != ATARI_MACH_MEDUSA) {
	/* add ST-RAM */
	ADD_CHUNK( 0, *phystop,
		   force_st_size, "ST-RAM" );
        bi.num_memory = chunk;
	stram_chunk = chunk - 1; /* remember ST-RAM block index */

	/* If the user wants the kernel to reside in ST-RAM, put the ST-RAM
	 * block first in the list of mem blocks; the kernel is always located
	 * in the first block. */
	if (load_to_stram && chunk > 1) {
	    struct mem_info temp = bi.memory[chunk - 1];
	    bi.memory[chunk - 1] = bi.memory[0];
	    bi.memory[0] = temp;
	    stram_chunk = 0;	/* ST-RAM block index changed */
	}
    }

    if (extramem_start && extramem_size) {
	ADD_CHUNK( extramem_start, extramem_size,
		   -1, "User-specified alternate RAM" );
    }
    
    /* verify that there is enough RAM; ST- and TT-RAM combined */
    if (total < MIN_RAMSIZE)
	ERROR( "Not enough RAM (need %d MB). Aborting...", MIN_RAMSIZE/MB );
    printf( "Total %s MB\n", format_mb(total) );
}

static char *format_mb( unsigned long size )
{
    static char buf[12];

    sprintf( buf, "%ld", size/MB );
    if (size % MB) {
	size /= 1024;
	sprintf( buf+strlen(buf), ".%02ld", (size*100+1024/2)/1024 );
    }
    return( buf );
}


/* getcookie -- function to get the value of the given cookie. */
static int getcookie(char *cookie, u_long *value)
{
    int i = 0;

    while(cookiejar[i] != 0L) {
	if(cookiejar[i] == *(u_long *)cookie) {
	    if (value)
	    	*value = cookiejar[i + 1];
	    return 1;
	}
	i += 2;
    }
    return -1;
}

int create_machspec_bootinfo(void)
{
    /* Atari tags */
    if (!add_bi_record(BI_ATARI_MCH_COOKIE, sizeof(bi.mch_cookie),
		       &bi.mch_cookie))
	return(0);
    if (!add_bi_record(BI_ATARI_MCH_TYPE, sizeof(bi.mch_type),
		       &bi.mch_type))
	return(0);
    return(1);
}

void compat_create_machspec_bootinfo(void)
{
    compat_bootinfo.bi_atari.hw_present = 0;
    compat_bootinfo.bi_atari.mch_cookie = bi.mch_cookie;
}

/*
 * Copy the kernel and the ramdisk to their final resting places.
 *
 * I assume that the kernel data and the ramdisk reside somewhere
 * in the middle of the memory.
 *
 * This program itself should be somewhere in the first 4096 bytes of memory
 * where the kernel never will be. In this way it can never be overwritten
 * by itself.
 *
 * At this point the registers have:
 * a0: the start of the final kernel
 * a1: the start of the current kernel
 * a2: the end of the final ramdisk
 * a3: the end of the current ramdisk
 * d0: the kernel size
 * d1: the ramdisk size 
 */
asm ("
.text
_copyall:

	movel	a0,a4	    	/* save the start of the kernel for booting */

1:	movel	a1@+,a0@+   	/* copy the kernel starting at the beginning */
	subql	#4,d0
	jcc	1b

	tstl	d1
	beq		3f

2:	movel	a3@-,a2@-   	/* copy the ramdisk starting at the end */
	subql	#4,d1
	jcc	2b

3:	jmp	a4@ 	    	/* jump to the start of the kernel */
_copyallend:
");

static int test_cpu_type (void)
{
    int rv;
    static char testarray[4] = { 0, 0, 1, 1 };
    
    __asm__ __volatile__ (
	"movel	0x2c,a0\n\t"		/* save line F vector */
	"movel	sp,a1\n\t"		/* save stack pointer */
	"movew	sr,d2\n\t"		/* save sr */
	"orw	#0x700,sr\n\t"		/* disable interrupts */
	"moveq	#0,%0\n\t"		/* assume 68000 (or 010) */

	"moveq	#1,d1\n\t"
	"tstb	%1@(d1:l:2)\n\t"	/* pre-020 CPU ignores scale and reads
					 * 0, otherwise 1 */
	"beq	1f\n\t"			/* if 0 is 68000, end */
	"moveq	#20,%0\n\t"		/* now assume 68020 */

	"movel	#2f,0x2c\n\t"	 	/* continue if trap */
	"movel	%1,a2\n\t"
	"nop	\n\t"
	".long	0xf0120a00\n\t"		/* pmove tt0,a2@, tt0 only on '030 */
	"nop	\n\t"
	"moveq	#30,%0\n\t"		/* surely is 68030 */
	"bra	1f\n"
	"2:\t"
					/* now could be '020 or '040+ */
	"movel	#1f,0x2c\n\t"	 	/* end if trap */
	"movel	%1,a2\n\t"
	"nop	\n\t"
	".long	0xf622a000\n\t"		/* move16 a2@+,a2@+ only on '040+ */
	"nop	\n\t"
	"moveq	#40,%0\n\t"		/* assume 68040 */

	"nop	\n\t"
	".word	0xf5ca\n\t"		/* plpar a2@, only on '060 */
	"nop	\n\t"
	"moveq	#60,%0\n"		/* surely is 68060 */

	"1:\t"
	"movel	a1,sp\n\t"		/* restore stack */
	"movel	a0,0x2c\n\t"		/* restore line F vector */
	"movew	d2,sr"			/* reenable ints */
	: "=&d" (rv)
	: "a" (testarray)
	: "d1", "d2", "a0", "a1", "a2", "memory" );
    return( rv );
}


/* Test if FPU instructions are executed in hardware, or if they're
   emulated in software.  For this, the F-line vector is temporarily
   replaced. */

static int test_software_fpu(void)
{
    int rv;
    
    __asm__ __volatile__
	( "movel	0x2c,a0\n\t"	/* save line F vector */
	  "movel	sp,a1\n\t"	/* save stack pointer */
	  "movew	sr,d2\n\t"	/* save sr */
	  "orw		#0x700,sr\n\t"	/* disable interrupts */
	  "movel	#1f,0x2c\n\t" /* new line F vector */
	  "moveq	#1,%0\n\t"	/* assume no FPU */
	  "fnop 	\n\t"		/* exec one FPU insn */
	  "nop		\n\t"
	  "moveq	#0,%0\n"	/* if come here, is a hard FPU */
	  "1:\t"
	  "movel	a1,sp\n\t"	/* restore stack */
	  "movel	a0,0x2c\n\t"	/* restore line F vector */
	  "movew	d2,sr"		/* reenable ints */
	  : "=d" (rv)
	  : /* no inputs */
	  : "d2", "a0", "a1" );
    return rv;
}


static void get_medusa_bank_sizes( u_long *bank1, u_long *bank2 )
{
    static u_long save_addr;
    u_long test_base, save_dtt0, saved_contents[16];
#define	TESTADDR(i)	(*((u_long *)((char *)test_base + i*8*MB)))
#define	TESTPAT		0x12345678
    unsigned short oldflags;
    int i;

    /* This ensures at least that none of the test addresses conflicts
     * with the test code itself; assume that MMU programming is modulo 8MB. */
    test_base = ((unsigned long)&save_addr & 0x007fffff) | 0x20000000;
    *bank1 = *bank2 = 0;
	
    /* Interrupts must be disabled because arbitrary addresses may be
     * temporarily overwritten, even code of an interrupt handler */
    __asm__ __volatile__ ( "movew sr,%0; oriw #0x700,sr" : "=g" (oldflags) : );
    disable_cache();
    /* make transparent translation for 0x2xxxxxxx area, enabled for
     * super+user, non-cacheable/serialized */
    __asm__ __volatile__ (
	".chip 68040\n\t"
	"movec	%/dtt0,%0\n\t"
	"movec	%1,%/dtt0\n\t"
	"nop\n\t"
	".chip 68k"
	: "=&d" (save_dtt0)
	: "d" (0x200fe040) );
	
    /* save contents of the test addresses */
    for( i = 0; i < 16; ++i )
	saved_contents[i] = TESTADDR(i);
	
    /* write 0s into all test addresses */
    for( i = 0; i < 16; ++i )
	TESTADDR(i) = 0;

    /* test for bank 1 */
#if 0
    /* This is Freddi's original test, but it didn't work. */
    TESTADDR(0) = TESTADDR(1) = TESTPAT;
    if (TESTADDR(1) == TESTPAT) {
	if (TESTADDR(2) == TESTPAT)
	    *bank1 = 8*MB;
	else if (TESTADDR(3) == TESTPAT)
	    *bank1 = 16*MB;
	else
	    *bank1 = 32*MB;
    }
    else {
	if (TESTADDR(2) == TESTPAT)
	    *bank1 = 0;
	else
	    *bank1 = 16*MB;
    }
#else
    TESTADDR(0) = TESTPAT;
    if (TESTADDR(1) == TESTPAT)
	*bank1 = 8*MB;
    else if (TESTADDR(2) == TESTPAT)
	*bank1 = 16*MB;
    else if (TESTADDR(4) == TESTPAT)
	*bank1 = 32*MB;
    else
	*bank1 = 64*MB;
#endif

    /* test for bank2 */
    if (TESTADDR(8) != 0)
	*bank2 = 0;
    else {
	TESTADDR(8) = TESTPAT;
	if (TESTADDR(9) != 0) {
	    if (TESTADDR(10) == TESTPAT)
		*bank2 = 8*MB;
	    else
		*bank2 = 32*MB;
	}
	else {
	    TESTADDR(9) = TESTPAT;
	    if (TESTADDR(10) == TESTPAT)
		*bank2 = 16*MB;
	    else
		*bank2 = 64*MB;
	}
    }
	
    /* restore contents of the test addresses and restore interrupt mask */
    for( i = 0; i < 16; ++i )
	TESTADDR(i) = saved_contents[i];
    /* remove transparent mapping */
    __asm__ __volatile__ (
	".chip 68040\n\t"
	"movec	%0,%/dtt0\n\t"
	"nop\n\t"
	".chip 68k"
	: /* no outputs */
	: "d" (save_dtt0) );
    __asm__ __volatile__ ( "movew %0,sr" : : "g" (oldflags) );
}

#undef TESTADDR
#undef TESTPAT


#define AB40_FAST_START	0x01000000
#define STEPSIZE	(4*MB)
#define BANKSIZE	(32*MB)
#define BANK_PAGES	(BANKSIZE/STEPSIZE)

static __inline__ unsigned long read_transparent( unsigned long addr )
{
    unsigned long ttreg_val, save_dtt0, val;
    
    ttreg_val = (addr & 0xff000000) | 0xe040;
    	/* 0xe040: enable super+user, non-cacheable/serialized */
    __asm__ __volatile__ (
	".chip 68040\n\t"
	"movec	%/dtt0,%0\n\t"
	"movec	%2,%/dtt0\n\t"
	"nop\n\t"
	"movel	%3@,%1\n\t"
	"nop\n\t"
	"movec	%0,%/dtt0\n\t"
	".chip 68k"
	: "=&d" (save_dtt0), "=d" (val)
	: "d" (ttreg_val), "a" (addr) );
    return( val );
}

static __inline__ void write_transparent( unsigned long addr,
										  unsigned long val )
{
    unsigned long ttreg_val, save_dtt0;
	
    ttreg_val = (addr & 0xff000000) | 0xe040;
    	/* 0xe040: enable super+user, non-cacheable/serialized */
    __asm__ __volatile__ (
	".chip 68040\n\t"
	"movec	%/dtt0,%0\n\t"
	"movec	%1,%/dtt0\n\t"
	"nop\n\t"
	"movel	%3,%2@\n\t"
	"nop\n\t"
	"movec	%0,%/dtt0\n\t"
	".chip 68k"
	: "=&d" (save_dtt0)
	: "d" (ttreg_val), "a" (addr), "d" (val) );
}

static __inline__ void pflusha( void )
{
    __asm__ __volatile__ ( ".chip 68040; nop; pflusha; nop; .chip 68k" );
}

static int get_ab040_bank_sizes( int maxres, u_long *result )
{
    unsigned long addr, addr2, val, npages, start, end;
    unsigned long saved_contents[2*BANK_PAGES];
    unsigned short oldflags;
    int n_result = 0;

    __asm__ __volatile__ ( "movew %/sr,%0; orw #0x700,%/sr"
			   : "=d" (oldflags) );

    pflusha();
    /* write test patterns at addresses */
    for( addr = AB40_FAST_START+2*BANKSIZE, npages = 2*BANK_PAGES;
	 npages; --npages ) {
	addr -= STEPSIZE;
	saved_contents[npages-1] = read_transparent( addr );
	write_transparent( addr, addr );
    }
    pflusha();

    addr = AB40_FAST_START;
    npages = 2*BANK_PAGES;
    while( npages ) {
	val = read_transparent( addr2 = addr );
	--npages;
	addr += STEPSIZE;
	if (val != addr2)
	    continue;
	/* note start addr of real mem (first addr where can read back) */
	start = addr2;
	/* loop while can read back, i.e. memory valid */
	do {
	    val = read_transparent( addr2 = addr );
	    --npages;
	    addr += STEPSIZE;
	    if (val != addr2)
		break;
	} while( npages );
	end = npages ? addr2 : addr;

	if (end - start >= MB) {
	    if (maxres-- > 0) {
		*result++ = start;
		*result++ = end - start;
		++n_result;
	    }
	    else
		npages = 0;
	}
    }

    /* restore previous contents */
    for( addr = AB40_FAST_START+2*BANKSIZE, npages = 2*BANK_PAGES;
	 npages; --npages ) {
	addr -= STEPSIZE;
	write_transparent( addr, saved_contents[npages-1] );
    }
    pflusha();
    
    __asm__ __volatile__ ( "movew %0,%/sr" : : "d" (oldflags) );
    return( n_result );
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
