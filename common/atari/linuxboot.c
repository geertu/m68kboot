/*
 * linuxboot.c -- Do actual booting of Linux kernel
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
 * $Id: linuxboot.c,v 1.2 1997-07-16 09:10:37 rnhodek Exp $
 * 
 * $Log: linuxboot.c,v $
 * Revision 1.2  1997-07-16 09:10:37  rnhodek
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

#if 0
#define _LINUX_TYPES_H		/* Hack to prevent including <linux/types.h> */
#include <asm/bootinfo.h>
#include <asm/setup.h>
#endif
#include <asm/page.h>

#include "linuxboot.h"
#include "loadkernel.h"
#include "bootinf.h"
#include "stream.h"
#include "inline-funcs.h"

#define MIN_RAMSIZE     (3)	/* 3 MB */
#define TEMP_STACKSIZE 256

/* This is missing in <unistd.h> */
extern int sync (void);

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
int force_st_size = -1;		/* force size of ST RAM (-1=autodetect) */
int force_tt_size = -1;		/* force size of TT RAM (-1=autodetect) */
unsigned long extramem_start=0;	/* start of extra memory block (0=none) */
unsigned long extramem_size=0;	/* size of that block */
char *kernel_name = "vmlinux";	/* name of kernel image */
char *ramdisk_name = NULL;	/* name of ramdisk image */
char command_line[CL_SIZE];	/* kernel command line */

unsigned long userstk;

/* these are defined in the mover code */
extern char copyall, copyallend;

/* TOS system variables */
#define phystop    ((unsigned long *)0x42e)
#define _p_cookies ((unsigned long **)0x5a0)
#define ramtop     ((unsigned long *)0x5a4)

/***************************** Prototypes *****************************/

static void get_cpu_infos( void );
static void get_mem_infos( void );
static char *format_mb( unsigned long size );
static int getcookie( char *cookie, u_long *value);
static int test_medusa( void );
static int test_software_fpu( void);
static void get_medusa_bank_sizes( u_long *bank1, u_long *bank2 );

/************************* End of Prototypes **************************/


#define ERROR(fmt,rest...)			\
    do {					\
	fprintf( stderr, fmt, ##rest );		\
	boot_exit( EXIT_FAILURE );		\
    } while(0)

#define SERROR(fmt,rest...)			\
    do {					\
	fprintf( stderr, fmt, ##rest );		\
	sclose();				\
	boot_exit( EXIT_FAILURE );		\
    } while(0)


void linux_boot( void )
{
    char *kname;
    u_long mch_type;		/* machine type cookie */
    void *bi_ptr;		/* pointer to bootinfo */
    u_long start_mem, mem_size;	/* addr and size of mem chunk where the kernel
				 * goes to */
    u_long kernel_size;		/* size of kernel image */
    char *memptr;		/* addr and size of load region */
    u_long memreq;
    u_long rd_size;		/* size of ramdisk and array of pointers to
				 * its data */

    /* We have to access some system variables to get
     * the information we need, so we must switch to
     * supervisor mode first.
     */
    userstk = Super(0L);

    /* get the info we need from the cookie-jar */
    cookiejar = *_p_cookies;
    if(cookiejar == 0L)
	/* if we find no cookies, it's probably an ST */
	ERROR( "Error: No cookiejar found. Is this an ST?\n" );

    /* Here we used to warn if MiNT was running, but this proved to be
     * unnecessary */

    /* machine is Atari */
    bi.machtype = MACH_ATARI;

    /* Pass contents of the _MCH cookie to the kernel */
    getcookie("_MCH", &mch_type);
    bi.mch_cookie = mch_type;
    
    /* Copy command line options into the kernel command line */
    strcpy( bi.command_line, command_line );
    kname = kernel_name;
    if (strncmp( kernel_name, "local:", 6 ) == 0)
	kname += 6;
    if (strlen(kname)+12 < CL_SIZE-1) {
	if (*bi.command_line)
	    strcat( bi.command_line, " " );
	strcat( bi.command_line, "BOOT_IMAGE=" );
	strcat( bi.command_line, kname );
    }
    printf ("Kernel command line: %s\n", bi.command_line );

    get_cpu_infos();

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
    bi.ramdisk.size = rd_size = load_ramdisk( ramdisk_name );
    
    /* open the kernel image and analyze it */
    kernel_size = open_kernel( kernel_name );

    /* Locate ramdisk in dest. memory */
    if (rd_size) {
	if (rd_size + kernel_size > mem_size - MB/2 && bi.num_memory > 1)
	    /* If running low on ST ram load ramdisk into alternate ram.  */
	    bi.ramdisk.addr = (u_long) bi.memory[1].addr + bi.memory[1].size -
			      rd_size;
	else
	    /* Else hopefully there is enough ST ram. */
	    bi.ramdisk.addr = (u_long)start_mem + mem_size - rd_size;
    }
    
    /* create the bootinfo structure */
    if (!create_bootinfo())
	SERROR( "Couldn't create bootinfo\n" );

    memreq = kernel_size + bi_size;
#ifdef BOOTINFO_COMPAT_1_0
    if (sizeof(compat_bootinfo) > bi_size)
	memreq = kernel_size+sizeof(compat_bootinfo);
#endif /* BOOTINFO_COMPAT_1_0 */
    /* align load address of ramdisk image, read() is sloooow on odd addr. */
    memreq = ((memreq + 3) & ~3) + rd_size;
    
    /* allocate RAM for the kernel */
    if (!(memptr = malloc( memreq )))
	SERROR( "Unable to allocate memory for kernel\n" );

    /* clearing the kernel's memory perhaps avoids "uninitialized bss"
     * types of bugs... */
    memset(memptr, 0, memreq - rd_size);

    /* move ramdisk image to its final resting place */
    move_ramdisk( memptr+memreq-rd_size, rd_size );
    /* read the text and data segments from the kernel image */
    load_kernel( memptr );

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

    /* for those who want to debug */
    if (debugflag) {
	if (rd_size) {
	    printf ("ramdisk src at %#lx, size is %ld\n",
		    (u_long)memptr - rd_size, bi.ramdisk.size);
	    printf ("ramdisk dest is %#lx ... %#lx\n",
		    bi.ramdisk.addr, bi.ramdisk.addr + rd_size - 1 );
	}
	kernel_debug_infos( start_mem );
	printf ("boot_info is at %#lx\n",
		start_mem + kernel_size);

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
    u_long cpu_type, fpu_type;

    /* get _CPU, _FPU and _MCH */
    getcookie("_CPU", &cpu_type);
    getcookie("_FPU", &fpu_type);

    /* check if we are on a 68030/40 with FPU */
    if ((cpu_type != 30 && cpu_type != 40 && cpu_type != 60))
	ERROR( "Machine type currently not supported. Aborting..." );

    switch(cpu_type) {
      case  0:
      case 10: break;
      case 20: bi.cputype = CPU_68020; bi.mmutype = MMU_68851; break;
      case 30: bi.cputype = CPU_68030; bi.mmutype = MMU_68030; break;
      case 40: bi.cputype = CPU_68040; bi.mmutype = MMU_68040; break;
      case 60: bi.cputype = CPU_68060; bi.mmutype = MMU_68060; break;
      default:
	ERROR( "Error: Unknown CPU type. Aborting...\n" );
    }

    printf("CPU: %ld; ", cpu_type + 68000);
    printf("FPU: ");

    /* check for FPU; in case of a '040 or '060, don't look at _FPU itself,
     * some software may set it to wrong values (68882 or the like) */
    if (cpu_type == 40) {
	bi.fputype = FPU_68040;
	puts( "68040" );
    }
    else if (cpu_type == 60) {
	bi.fputype = FPU_68060;
	puts( "68060" );
    }
    else {
	switch ((fpu_type >> 16) & 7) {
	  case 0:
	    puts("not present");
	    break;
	  case 1:
	    puts("SFP004 not supported. Assuming no FPU.");
	    break;
	  case 2:
	    /* try to determine real type */
	    if (fpu_idle_frame_size () != 0x18)
		goto m68882;
	    /* fall through */
	  case 4:
	    bi.fputype = FPU_68881;
	    puts("68881");
	    break;
	  case 6:
	  m68882:
	    bi.fputype = FPU_68882;
	    puts("68882");
	    break;
	  default:
	    puts("Unknown FPU type. Assuming no FPU.");
	    break;
	}
    }
    
    /* ++roman: If an FPU was announced in the cookie, test
       whether it is a real hardware FPU or a software emulator!  */
    if (bi.fputype) {
	if (test_software_fpu()) {
	    bi.fputype = 0;
	    puts("FPU: software emulated. Assuming no FPU.");
	}
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
    
    if (!test_medusa()) {

	/* TT RAM tests for non-Medusa machines */
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
	    if (*ramtop)
		/* the 'ramtop' variable at 0x05a4 is not
		 * officially documented. We use it anyway
		 * because it is the only way to get the TTram size.
		 * (It is zero if there is no TTram.)
		 */
		ADD_CHUNK( TT_RAM_BASE, *ramtop - TT_RAM_BASE,
			   force_tt_size, "TT-RAM" );
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

	/* add ST-RAM */
	ADD_CHUNK( 0, *phystop,
		   force_st_size, "ST-RAM" );
        bi.num_memory = chunk;

	/* If the user wants the kernel to reside in ST-RAM, put the ST-RAM
	 * block first in the list of mem blocks; the kernel is always located
	 * in the first block. */
	if (load_to_stram && chunk > 1) {
	    struct mem_info temp = bi.memory[chunk - 1];
	    bi.memory[chunk - 1] = bi.memory[0];
	    bi.memory[0] = temp;
	}
    }
    else {

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

/* Test for a Medusa: This is the only machine on which address 0 is
 * writeable!
 * ...err! On the Afterburner040 (for the Falcon) it's the same... So we do
 * another test with 0x00ff82fe, that gives a bus error on the Falcon, but is
 * in the range where the Medusa always asserts DTACK.
 * On the Hades address 0 is writeable as well and it asserts DTACK on
 * address 0x00ff82fe. To test if the machine is a Hades, address 0xb0000000
 * is tested. On the Medusa this gives a bus error.
 */

static int test_medusa( void )
{
    int rv = 0;

    __asm__ __volatile__
	( "movel	0x8,a0\n\t"
	  "movel	sp,a1\n\t"
	  "moveb	0x0,d1\n\t"
	  "movel	#Lberr,0x8\n\t"
	  "moveq	#0,%0\n\t"
	  "clrb		0x0\n\t"
	  "nop		\n\t"
	  "moveb	d1,0x0\n\t"
	  "nop		\n\t"
	  "tstb		0x00ff82fe\n\t"
	  "nop		\n\t"
	  "moveq	#1,%0\n\t"
	  "tstb		0xb0000000\n\t"
	  "nop		\n\t"
	  "moveq	#0,%0\n"
	  "Lberr:\t"
	  "movel	a1,sp\n\t"
	  "movel	a0,0x8"
	  : "=d" (rv)
	  : /* no inputs */
	  : "d1", "a0", "a1", "memory" );
    return( rv );
}

/* Test if FPU instructions are executed in hardware, or if they're
   emulated in software.  For this, the F-line vector is temporarily
   replaced. */

static int test_software_fpu(void)
{
    int rv = 0;
    
    __asm__ __volatile__
	( "movel	0x2c,a0\n\t"
	  "movel	sp,a1\n\t"
	  "movel	#Lfline,0x2c\n\t"
	  "moveq	#1,%0\n\t"
	  "fnop 	\n\t"
	  "nop		\n\t"
	  "moveq	#0,%0\n"
	  "Lfline:\t"
	  "movel	a1,sp\n\t"
	  "movel	a0,0x2c"
	  : "=d" (rv)
	  : /* no inputs */
	  : "a0", "a1" );
    return rv;
}


static void get_medusa_bank_sizes( u_long *bank1, u_long *bank2 )
{
    static u_long save_addr;
    u_long test_base, saved_contents[16];
#define	TESTADDR(i)	(*((u_long *)((char *)test_base + i*8*MB)))
#define	TESTPAT		0x12345678
    unsigned short oldflags;
    int i;

    /* This ensures at least that none of the test addresses conflicts
     * with the test code itself */
    test_base = ((unsigned long)&save_addr & 0x007fffff) | 0x20000000;
    *bank1 = *bank2 = 0;
	
    /* Interrupts must be disabled because arbitrary addresses may be
     * temporarily overwritten, even code of an interrupt handler */
    __asm__ __volatile__ ( "movew sr,%0; oriw #0x700,sr" : "=g" (oldflags) : );
    disable_cache();
	
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
    __asm__ __volatile__ ( "movew %0,sr" : : "g" (oldflags) );
}

#undef TESTADDR
#undef TESTPAT


/* Local Variables: */
/* tab-width: 8     */
/* End:             */
