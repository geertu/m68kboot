/*
 *  linux/arch/m68k/boot/amiga/linuxboot.c -- Generic routine to boot Linux/m68k
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
 *  History:
 *	11 Jun 1997 Fix for unpadded gzipped ramdisks with bootinfo interface
 *		    version 1.0
 *	27 Mar 1997 FPU-less machines couldn't boot kernels that use bootinfo
 *		    interface version 1.0 (Geert)
 *	 2 Mar 1997 Updated for the new Zorro ID scheme
 *	 3 Feb 1997 Implemented kernel decompression (Geert, based on Roman's
 *		    code for ataboot)
 *	30 Dec 1996 Reverted the CPU detection to the old scheme
 *		    New boot parameter override scheme (Geert)
 *      27 Nov 1996 Compatibility with bootinfo interface version 1.0 (Geert)
 *       9 Sep 1996 Rewritten option parsing
 *		    New parameter passing to linuxboot() (linuxboot_args)
 *		    (Geert)
 *	18 Aug 1996 Updated for the new boot information structure (Geert)
 *	10 Jan 1996 The real Linux/m68k boot code moved to linuxboot.[ch]
 *		    (Geert)
 *	11 Jul 1995 Support for ELF kernel (untested!) (Andreas)
 *	 7 Mar 1995 Memory block sizes are rounded to a multiple of 256K
 *		    instead of 1M (Geert)
 *	31 May 1994 Memory thrash problem solved (Geert)
 *	11 May 1994 A3640 MapROM check (Geert)
 * 
 * $Id: linuxboot.c,v 1.11 2004-08-15 12:35:12 geert Exp $
 * 
 * $Log: linuxboot.c,v $
 * Revision 1.11  2004-08-15 12:35:12  geert
 * Add curly braces to keep new gcc happy
 *
 * Revision 1.10  1998/04/06 01:40:54  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.9  1998/02/26 10:04:52  rnhodek
 * Also set BOOT_IMAGE= command line option on Amiga.
 *
 * Revision 1.8  1997/09/19 09:06:40  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.7  1997/08/10 19:22:55  rnhodek
 * Moved AmigaOS inline funcs to extr header inline-funcs.h; the functions
 * can't be compiled under Linux
 *
 * Revision 1.6  1997/08/10 19:03:41  rnhodek
 * Removed ALIGN_STR to avoid dependency on <linux/linkage.h>
 *
 * Revision 1.5  1997/07/18 12:10:37  rnhodek
 * Call open_ramdisk only if ramdisk_name set; 0 return value means error.
 * Rename load_ramdisk/move_ramdisk to open_ramdisk/load_ramdisk, in parallel
 * to the *_kernel functions.
 * Rewrite open/load_ramdisk so that the temp storage and additional memcpy
 * are avoided if file size known after sopen().
 *
 * Revision 1.4  1997/07/17 14:18:54  geert
 * Integrate amiboot 5.6 changes (compr. ramdisk and 2.0 kernel)
 *
 * Revision 1.3  1997/07/16 14:05:06  rnhodek
 * Sorted out which headers to use and the like; Amiga bootstrap now compiles.
 * Puts and other generic functions now defined in bootstrap.h
 *
 * Revision 1.2  1997/07/16 09:11:00  rnhodek
 * Made compat_create_machspec_bootinfo return void
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */


#ifndef __GNUC__
#error GNU CC is required to compile this program
#endif /* __GNUC__ */


#include <stddef.h>
#ifdef IN_BOOTSTRAP
#include <string.h>
#undef SYMBOL_NAME_STR
#define SYMBOL_NAME_STR(X) "_"#X
#else
#ifdef IN_LILO
#include "strlib.h"
#include <linux/linkage.h>
#endif
#endif
#include <errno.h>
#include <sys/types.h>

#include <linux/version.h>
#include <asm/amigahw.h>
#include <asm/page.h>

#include "linuxboot.h"
#include "bootstrap.h"
#include "loadkernel.h"
#include "bootinf.h"
#include "inline-funcs.h"


#undef custom
#define custom ((*(volatile struct CUSTOM *)(CUSTOM_PHYSADDR)))

/* temporary stack size */
#define TEMP_STACKSIZE	(256)

#define DEFAULT_BAUD	(9600)

extern char copyall, copyallend;

static const struct linuxboot_args *linuxboot_args;

/* Bootinfo */
struct amiga_bootinfo bi;
unsigned long bi_size;
union _bi_union bi_union;

#ifdef BOOTINFO_COMPAT_1_0
struct compat_bootinfo compat_bootinfo;
#endif /* BOOTINFO_COMPAT_1_0 */

#define kernelname	linuxboot_args->kernelname
#define ramdiskname	linuxboot_args->ramdiskname
#define debugflag	linuxboot_args->debugflag
#define keep_video	linuxboot_args->keep_video
#define reset_boards	linuxboot_args->reset_boards
#define baud		linuxboot_args->baud

#define Sleep		linuxboot_args->sleep


    /*
     *  Function Prototypes
     */

static u_long get_chipset(void);
static void get_processor(u_long *cpu, u_long *fpu, u_long *mmu);
static u_long get_model(u_long chipset);
static int probe_resident(const char *name);
static int probe_resource(const char *name);
static void start_kernel(void (*startfunc)(), char *stackp, char *memptr,
			 u_long start_mem, u_long kernel_size, u_long rd_dest,
			 u_long rd_size) __attribute__ ((noreturn));
u_long maprommed(void);

    /*
     *	Reset functions for nasty Zorro boards
     */

static void reset_rb3(const struct ConfigDev *cd);
static void reset_piccolo(const struct ConfigDev *cd);
static void reset_sd64(const struct ConfigDev *cd);
static void reset_ariadne(const struct ConfigDev *cd);
static void reset_hydra(const struct ConfigDev *cd);
#if 0
static void reset_a2060(const struct ConfigDev *cd);
#endif

struct boardreset {
#if LINUX_VERSION_CODE >= 0x02012a
    zorro_id id;
#else /* old Zorro ID scheme */
    u_short manuf;
    u_short prod;
#endif /* old Zorro ID scheme */
    const char *name;
    void (*reset)(const struct ConfigDev *cd);
};

static struct boardreset boardresetdb[] = {
#if LINUX_VERSION_CODE >= 0x02012a
    { ZORRO_PROD_HELFRICH_RAINBOW_III, "Rainbow 3", reset_rb3 },
    { ZORRO_PROD_HELFRICH_PICCOLO_REG, "Piccolo", reset_piccolo },
    { ZORRO_PROD_HELFRICH_SD64_REG, "SD64", reset_sd64 },
    { ZORRO_PROD_VILLAGE_TRONIC_ARIADNE, "Ariadne", reset_ariadne },
    { ZORRO_PROD_HYDRA_SYSTEMS_AMIGANET, "Hydra", reset_hydra },
#if 0
    { ZORRO_PROD_CBM_A2060, "A2060", reset_a2060 },
#endif
#else /* old Zorro ID scheme */
    { MANUF_HELFRICH1, PROD_RAINBOW3, "Rainbow 3", reset_rb3 },
    { MANUF_HELFRICH2, PROD_PICCOLO_REG, "Piccolo", reset_piccolo },
    { MANUF_HELFRICH2, PROD_SD64_REG, "SD64", reset_sd64 },
    { MANUF_VILLAGE_TRONIC, PROD_ARIADNE, "Ariadne", reset_ariadne },
    { MANUF_HYDRA_SYSTEMS, PROD_AMIGANET, "Hydra", reset_hydra },
#if 0
    { MANUF_COMMODORE, PROD_A2060, "A2060", reset_a2060 },
#endif
#endif /* old Zorro ID scheme */
};
#define NUM_BOARDRESET	sizeof(boardresetdb)/sizeof(*boardresetdb)

static void (*boardresetfuncs[ZORRO_NUM_AUTO])(const struct ConfigDev *cd);


const char *amiga_models[] = {
    "Amiga 500", "Amiga 500+", "Amiga 600", "Amiga 1000", "Amiga 1200",
    "Amiga 2000", "Amiga 2500", "Amiga 3000", "Amiga 3000T", "Amiga 3000+",
    "Amiga 4000", "Amiga 4000T", "CDTV", "CD32", "Draco"
};
const u_long first_amiga_model = AMI_500;
const u_long last_amiga_model = AMI_DRACO;


#define MASK(model)	(1<<AMI_##model)

#define CLASS_A3000	(MASK(3000) | MASK(3000T))
#define CLASS_A4000	(MASK(4000) | MASK(4000T))
#define CLASS_ZKICK	(MASK(500) | MASK(1000) | MASK(2000) | MASK(2500))


    /*
     *	Boot the Linux/m68k Operating System
     */

u_long linuxboot(const struct linuxboot_args *args)
{
    int do_fast, do_chip;
    int i, j;
    const struct MemHeader *mnp;
    struct ConfigDev *cdp = NULL;
    char *memptr = NULL;
    u_long *stack = NULL;
    u_long fast_total, model_mask, startcodesize, start_mem, mem_size, rd_size;
    u_long kernel_size;
    u_int realbaud;
    u_long memreq = 0;
    void (*startfunc)(void);
    u_short manuf;
    u_char prod;
    void *bi_ptr;
    char *kname;

    linuxboot_args = args;

    /* print the greet message */
    Puts("\nLinux/m68k Amiga Bootstrap version " AMIBOOT_VERSION "\n");
    Puts("Copyright 1993,1994 by Hamish Macdonald and Greg Harp\n\n");

    /* Note: Initial values in bi override detected values */
    bi = args->bi;

    /* machine is Amiga */
    bi.machtype = MACH_AMIGA;

    /* determine chipset */
    if (!bi.chipset)
	bi.chipset = get_chipset();

    /* determine CPU, FPU and MMU type */
    if (!bi.cputype)
	get_processor(&bi.cputype, &bi.fputype, &bi.mmutype);

    /* determine Amiga model */
    if (!bi.model)
	bi.model = get_model(bi.chipset);
    model_mask = (bi.model != AMI_UNKNOWN) ? 1<<bi.model : 0;

    /* Memory & AutoConfig based on 'unix_boot.c' by C= */

    /* find all of the autoconfig boards in the system */
    if (!bi.num_autocon) {
	for (i = 0; (cdp = (struct ConfigDev *)FindConfigDev(cdp, -1, -1)); i++)
	    if (bi.num_autocon < ZORRO_NUM_AUTO)
		/* copy the contents of each structure into our boot info and
		   count this device */
		memcpy(&bi.autocon[bi.num_autocon++], cdp,
		       sizeof(struct ConfigDev));
	    else
		Printf("Warning: too many AutoConfig devices. Ignoring device at "
		       "0x%08lx\n", (u_long)cdp->cd_BoardAddr);
    }

    do_fast = bi.num_memory ? 0 : 1;
    do_chip = bi.chip_size ? 0 : 1;
    /* find out the memory in the system */
    for (mnp = (struct MemHeader *)SysBase->MemList.lh_Head;
	 mnp->mh_Node.ln_Succ;
	 mnp = (struct MemHeader *)mnp->mh_Node.ln_Succ) {
	struct MemHeader mh;

	/* copy the information */
	mh = *mnp;

	/* skip virtual memory */
	if (!(mh.mh_Attributes & MEMF_PUBLIC))
	    continue;

	/* if we suspect that Kickstart is shadowed in an A3000,
	   modify the entry to show 512K more at the top of RAM
	   Check first for a MapROMmed A3640 board: overwriting the
	   Kickstart image causes an infinite lock-up on reboot! */
	if ((mh.mh_Upper == (void *)0x07f80000) &&
	    (model_mask & (CLASS_A3000 | CLASS_A4000))) {
	    if ((bi.cputype & CPU_68040) && Supervisor(maprommed))
		Puts("A3640 MapROM detected.\n");
	    else if (model_mask & CLASS_A3000) {
		mh.mh_Upper = (void *)0x08000000;
		Puts("A3000 shadowed Kickstart detected.\n");
	    }
	}

	/* if we suspect that Kickstart is zkicked,
	   modify the entry to show 512K more at the botton of RAM */
	if ((mh.mh_Lower == (void *)0x00280020) &&
	    (model_mask & CLASS_ZKICK)) {
	    mh.mh_Lower = (void *)0x00200000;
	    Puts("ZKick detected.\n");
	}

	/* mask the memory limit values */
	mh.mh_Upper = (void *)((u_long)mh.mh_Upper & 0xfffff000);
	mh.mh_Lower = (void *)((u_long)mh.mh_Lower & 0xfffff000);

	/* if fast memory */
	if (do_fast && mh.mh_Attributes & MEMF_FAST) {
	    /* set the size value to the size of this block and mask off to a
	       256K increment */
	    u_long size = ((u_long)mh.mh_Upper-(u_long)mh.mh_Lower)&0xfffc0000;
	    if (size > 0) {
		if (bi.num_memory < NUM_MEMINFO) {
		    /* record the start and size */
		    bi.memory[bi.num_memory].addr = (u_long)mh.mh_Lower;
		    bi.memory[bi.num_memory].size = size;
		    /* count this block */
		    bi.num_memory++;
		} else
		    Printf("Warning: too many memory blocks. Ignoring block "
		    	   "of %ldK at 0x%08lx\n", size>>10,
			   (u_long)mh.mh_Lower);
	    }
	} else if (do_chip && mh.mh_Attributes & MEMF_CHIP)
	    /* if CHIP memory, record the size */
	    bi.chip_size = (u_long)mh.mh_Upper;
    }

    /* get info from ExecBase */
    if (!bi.vblank)
	bi.vblank = SysBase->VBlankFrequency;
    if (!bi.psfreq)
	bi.psfreq = SysBase->PowerSupplyFrequency;
    if (!bi.eclock)
	bi.eclock = SysBase->ex_EClockFrequency;

    /* serial port */
    if (!bi.serper) {
	realbaud = baud ? baud : DEFAULT_BAUD;
	bi.serper = (5*bi.eclock+realbaud/2)/realbaud-1;
    }

    /* display Amiga model */
    if (bi.model >= first_amiga_model && bi.model <= last_amiga_model)
	Printf("%s ", amiga_models[bi.model-first_amiga_model]);
    else
	Puts("Amiga ");

    /* display the CPU type */
    Puts("CPU: ");
    switch (bi.cputype) {
	case CPU_68020:
	    Puts("68020 (Do you have an MMU?)");
	    break;
	case CPU_68030:
	    Puts("68030");
	    break;
	case CPU_68040:
	    Puts("68040");
	    break;
	case CPU_68060:
	    Puts("68060");
	    break;
	default:
	    Puts("Insufficient for Linux.  Aborting...\n");
	    Printf("SysBase->AttnFlags = 0x%08x\n", SysBase->AttnFlags);
	    goto Fail;
    }
    switch (bi.fputype) {
	case FPU_68881:
	    Puts(" with 68881 FPU");
	    break;
	case FPU_68882:
	    Puts(" with 68882 FPU");
	    break;
	case FPU_68040:
	case FPU_68060:
	    Puts(" with internal FPU");
	    break;
	default:
	    Puts(" without FPU");
	    break;
    }

    /* display the chipset */
    switch (bi.chipset) {
	case CS_STONEAGE:
	    Puts(", old or unknown chipset");
	    break;
	case CS_OCS:
	    Puts(", OCS");
	    break;
	case CS_ECS:
	    Puts(", ECS");
	    break;
	case CS_AGA:
	    Puts(", AGA chipset");
	    break;
    }

    Puts("\n\n");

    /* add BOOT_IMAGE= argument if enough space left on command line */
    kname = (char *)kernelname;
    if (strncmp( kernelname, "local:", 6 ) == 0 ||
	strncmp( kernelname, "bootp:", 6 ) == 0)
	kname += 6;
    if (strlen(bi.command_line)+strlen(kname)+12 < CL_SIZE-1) {
	if (*bi.command_line)
	    strcat( bi.command_line, " " );
	strcat( bi.command_line, "BOOT_IMAGE=" );
	strcat( bi.command_line, kname );
    }
    /* display the command line */
    Printf("Command line is '%s'\n", bi.command_line);

    /* display the clock statistics */
    Printf("Vertical Blank Frequency: %dHz\n", bi.vblank);
    Printf("Power Supply Frequency: %dHz\n", bi.psfreq);
    Printf("EClock Frequency: %ldHz\n\n", bi.eclock);

    /* display autoconfig devices */
    if (bi.num_autocon) {
	Printf("Found %d AutoConfig Device%s\n", bi.num_autocon,
	       bi.num_autocon > 1 ? "s" : "");
	for (i = 0; i < bi.num_autocon; i++) {
	    Printf("Device %d: addr = 0x%08lx", i,
		   (u_long)bi.autocon[i].cd_BoardAddr);
	    boardresetfuncs[i] = NULL;
	    if (reset_boards) {
		manuf = bi.autocon[i].cd_Rom.er_Manufacturer;
		prod = bi.autocon[i].cd_Rom.er_Product;
		for (j = 0; j < NUM_BOARDRESET; j++) {
#if LINUX_VERSION_CODE >= 0x02012a
		    if ((manuf == ZORRO_MANUF(boardresetdb[j].id)) &&
			(prod == ZORRO_PROD(boardresetdb[j].id))) {
#else /* old Zorro ID scheme */
                    if ((manuf == boardresetdb[j].manuf) &&
                        (prod == boardresetdb[j].prod)) {
#endif /* old Zorro ID scheme */
			Printf(" [%s - will be reset at kernel boot time]",
			       boardresetdb[j].name);
			boardresetfuncs[i] = boardresetdb[j].reset;
			break;
		    }
		}
	    }
	    PutChar('\n');
	}
    } else
	Puts("No AutoConfig Devices Found\n");

    /* display memory */
    if (bi.num_memory) {
	Printf("\nFound %d Block%sof Memory\n", bi.num_memory,
	       bi.num_memory > 1 ? "s " : " ");
	for (i = 0; i < bi.num_memory; i++)
	    Printf("Block %d: 0x%08lx to 0x%08lx (%ldK)\n", i,
		   bi.memory[i].addr, bi.memory[i].addr+bi.memory[i].size,
		   bi.memory[i].size>>10);
    } else {
	Puts("No memory found?!  Aborting...\n");
	goto Fail;
    }

    /* display chip memory size */
    Printf("%ldK of CHIP memory\n", bi.chip_size>>10);

    start_mem = bi.memory[0].addr;
    mem_size = bi.memory[0].size;

    /* tell us where the kernel will go */
    Printf("\nThe kernel will be located at 0x%08lx\n", start_mem);

    /* verify that there is enough Chip RAM */
    if (bi.chip_size < 512*1024) {
	Puts("Not enough Chip RAM in this system.  Aborting...\n");
	goto Fail;
    }

    /* verify that there is enough Fast RAM */
    for (fast_total = 0, i = 0; i < bi.num_memory; i++)
	fast_total += bi.memory[i].size;
    if (fast_total < 2*1024*1024) {
	Puts("Not enough Fast RAM in this system.  Aborting...\n");
	goto Fail;
    }

    /* load the ramdisk */
    if (ramdiskname) {
	if (!(rd_size = open_ramdisk( ramdiskname )))
	    goto Fail;
    }
    else
	rd_size = 0;
    bi.ramdisk.size = rd_size;
    bi.ramdisk.addr = (u_long)start_mem+mem_size-rd_size;

    /* create the bootinfo structure */
    if (!create_bootinfo())
	goto Fail;

    /* open kernel executable and read exec header */
    if (!(kernel_size = open_kernel( kernelname )))
	goto Fail;

    /* Load the kernel at one page after start of mem */
    start_mem += PAGE_SIZE;
    mem_size -= PAGE_SIZE;

    memreq = kernel_size+bi_size+rd_size;
#ifdef BOOTINFO_COMPAT_1_0
    if (sizeof(compat_bootinfo) > bi_size)
	memreq = kernel_size+sizeof(compat_bootinfo)+rd_size;
#endif /* BOOTINFO_COMPAT_1_0 */
    if (!(memptr = (char *)AllocMem(memreq, MEMF_FAST | MEMF_PUBLIC |
					    MEMF_CLEAR))) {
	Puts("Unable to allocate memory\n");
	goto Fail;
    }

    /* read the text and data segments from the kernel image */
    if (!load_kernel( memptr ))
	goto Fail;

    /* Check kernel's bootinfo version */
    switch (check_bootinfo_version( memptr, MACH_AMIGA, AMIGA_BOOTI_VERSION,
				    COMPAT_AMIGA_BOOTI_VERSION )) {
	case BI_VERSION_MAJOR(AMIGA_BOOTI_VERSION):
	    bi_ptr = &bi_union.record;
	    break;

#ifdef BOOTINFO_COMPAT_1_0
	case BI_VERSION_MAJOR(COMPAT_AMIGA_BOOTI_VERSION):
	    if (!create_compat_bootinfo())
		goto Fail;
	    bi_ptr = &compat_bootinfo;
	    bi_size = sizeof(compat_bootinfo);
	    break;
#endif /* BOOTINFO_COMPAT_1_0 */

	default:
	    goto Fail;
    }

    /* copy the bootinfo to the end of the kernel image */
    memcpy((void *)(memptr+kernel_size), bi_ptr, bi_size);

    /* move ramdisk image to its final resting place */
    if (!load_ramdisk( ramdiskname, memptr+kernel_size+bi_size, rd_size ))
	goto Fail;

    /* allocate temporary chip ram stack */
    if (!(stack = (u_long *)AllocMem(TEMP_STACKSIZE, MEMF_CHIP | MEMF_CLEAR))) {
	Puts("Unable to allocate memory for stack\n");
	goto Fail;
    }

    /* allocate chip ram for copy of startup code */
    startcodesize = &copyallend-&copyall;
    if (!(startfunc = (void (*)(void))AllocMem(startcodesize,
					       MEMF_CHIP | MEMF_CLEAR))) {
	Puts("Unable to allocate memory for startcode\n");
	goto Fail;
    }

    /* copy startup code to CHIP RAM */
    memcpy(startfunc, &copyall, startcodesize);

    if (debugflag) {
	if (bi.ramdisk.size)
	    Printf("RAM disk at 0x%08lx, size is %ldK\n",
		   (u_long)memptr+kernel_size+bi_size, bi.ramdisk.size>>10);

	kernel_debug_infos( start_mem );

	Printf("ramdisk dest is 0x%08lx\n", bi.ramdisk.addr);
	Printf("ramdisk lower limit is 0x%08lx\n",
	       (u_long)memptr+kernel_size+bi_size);
	Printf("ramdisk src top is 0x%08lx\n",
	       (u_long)memptr+kernel_size+bi_size+rd_size);

	Puts("\nType a key to continue the Linux/m68k boot...");
	GetChar();
	PutChar('\n');
    }

    /* wait for things to settle down */
    Sleep(1000000);

    if (!keep_video)
	/* set graphics mode to a nice normal one */
	LoadView(NULL);

    Disable();

    /* reset nasty Zorro boards */
    if (reset_boards)
	for (i = 0; i < bi.num_autocon; i++)
	    if (boardresetfuncs[i])
		boardresetfuncs[i](&bi.autocon[i]);

    /* Turn off all DMA */
    custom.dmacon = DMAF_ALL | DMAF_MASTER;

    /* turn off caches */
    CacheControl(0, ~0);

    /* Go into supervisor state */
    SuperState();

    /* turn off any mmu translation */
    disable_mmu();

    /* execute the copy-and-go code (from CHIP RAM) */
    start_kernel(startfunc, (char *)stack+TEMP_STACKSIZE, memptr, start_mem,
		 kernel_size, bi.ramdisk.addr, rd_size);

    /* Clean up and exit in case of a failure */
Fail:
    if (memptr)
	FreeMem((void *)memptr, memreq);
    if (stack)
	FreeMem((void *)stack, TEMP_STACKSIZE);
    return(FALSE);
}


    /*
     *	Determine the Chipset
     */

static u_long get_chipset(void)
{
    u_char cs;
    u_long chipset;

    if (GfxBase->Version >= 39)
	cs = SetChipRev(SETCHIPREV_BEST);
    else
	cs = GfxBase->ChipRevBits0;
    if ((cs & GFXG_AGA) == GFXG_AGA)
	chipset = CS_AGA;
    else if ((cs & GFXG_ECS) == GFXG_ECS)
	chipset = CS_ECS;
    else if ((cs & GFXG_OCS) == GFXG_OCS)
	chipset = CS_OCS;
    else
	chipset = CS_STONEAGE;
    return(chipset);
}




/*
 *	Determine the CPU Type
 * Thanks to Roman Hodek and Andreas Schwab for the necessary hints to
 * get this going
 */


/*
 * Distinguish between 68030, 68040 and 68060 CPU
 */

/*
 * CPU type are no longer hardwired in this function
 *
 * STRINGIFY macros shamlessly stolen from module.h
 */

#define STRINGIFY(x) #x
#define STRINGIFY_CONTENT(x) STRINGIFY(x)

static __inline__ unsigned long ckcpu346( void)
{
register void *systack;
register unsigned long rv;
unsigned long temp1, temp2, temp3;

systack=SuperState();
__asm__ __volatile__ ("
| Register Usage:
| %0	return value (cpu type)
| %1	saved VBR
| %2	saved stack pointer
| %3	temporary copy of VBR

	.chip 68060
	movec	%%vbr,%1	| get vbr
	movew	%%sr,%-		| save IPL
	movel	%1@(11*4),%-	| save old trap vector (Line F)
	orw	#0x700,%%sr	| disable ints
	movel	#1f,%1@(11*4)	| set L1 as new vector
	movel	%%sp,%2		| save stack pointer
	moveql	%4,%0		| value with exception (030)
	movel	%1,%3		| we move the vbr to itself
	nop			| clear instruction pipeline
	move16	%3@+,%3@+	| the 030 test instruction
	nop			| clear instruction pipeline
	moveql	%5,%0		| value with exception (040)
	nop			| clear instruction pipeline
	plpar	%1@		| the 040 test instruction
	nop			| clear instruction pipeline
	moveql	%6,%0		| value if we come here (060)
1:	movel	%2,%%sp		| restore stack pointer
	movel	%+,%1@(11*4)	| restore trap vector
	movew	%+,%%sr		| restore IPL
	.chip 68k
" : "=d" (rv), "=a" (temp1), "=r" (temp2), "=a" (temp3)
  : "i" (CPU_68030), "i" (CPU_68040), "i" (CPU_68060));
if (systack)
	UserState(systack);
return rv;
}

/* Tests the presence of an FPU */

static __inline__ unsigned long ckfpu()
{
register void *systack;
register unsigned long rv;
unsigned long temp1, temp2;

systack=SuperState();
__asm__ __volatile__ ("
| register usage:
| %0 return value (0 no fpu, 1 fpu present)
| %1 saved VBR
| %2 saved stack pointer

	.chip 68060
	movec	%%vbr,%1	| get vbr
	movew	%%sr,%-		| save IPL
	movel	%1@(11*4),%-	| save old trap vector (Line F)
	orw	#0x700,%%sr	| diable ints
	movel	#1f,%1@(11*4)	| set L1 as new vector
	movel	%%sp,%2		| save stack pointer
	moveql	#0,%0		| value with exeption
	nop
	fnop			| is an FPU present ?
	nop
	moveql	#1,%0		| value if we come here
1:	movel	%2,%%sp		| restore stack pointer
	movel	%+,%1@(11*4)	| restore trap vector
	movew	%+,%%sr		| restore IPL
	.chip 68k
" : "=d" (rv), "=a" (temp1), "=r" (temp2) : /* no input */);

if (systack)
	UserState(systack);
return rv;
}

/*
 * get/set the PCR
 *
 * WARNING: Works only for 68060 and up
 *
 */

static unsigned long get_pcr()
{
register void *systack;
register unsigned long rv __asm__("d2");

systack=SuperState();
__asm__ __volatile__ ("
	.long 0x4e7a2808	| movec pcr,d2
	" : "=d" (rv) : /* no input */ );
if (systack)
	UserState(systack);
return rv;
}

static void set_pcr(unsigned long val)
{
register void *systack;
register u_long pcr __asm__("d2") = val;

systack=SuperState();
__asm__ __volatile__ ("
	.long 0x4e7b2808	| movec d2,pcr
	" : /* no output */ : "d" (pcr) );
if (systack)
	UserState(systack);
}

static void get_processor(u_long *cpu, u_long *fpu, u_long *mmu)
{
    unsigned long pcr;

    *cpu = *fpu = 0;

    if (SysBase->AttnFlags & AFF_68060)
	*cpu = CPU_68060;
    else if (SysBase->AttnFlags & AFF_68040)
	*cpu = CPU_68040;
    else if (SysBase->AttnFlags & AFF_68030)
	*cpu = CPU_68030;
    else if (SysBase->AttnFlags & AFF_68020)
	*cpu = CPU_68020;

    if (*cpu == CPU_68030 || *cpu == CPU_68040 || *cpu == CPU_68060)
        *cpu = ckcpu346();
    
    if (*cpu == CPU_68060) {
        pcr = get_pcr();
	/* Value for CS MK II '060 at this point: 0x0430102 */
	if ((pcr & 0xffff0000) != 0x04300000) {
	    /* has a pcr and is not an 68060 -> unknown cpu */
	    Printf("CPU not know -- Please report type and this value: pcr = 0x%08lx\n", pcr);
	    *cpu = *fpu = *mmu = 0;
	    return;
	}
 
	/* enable fpu if it's not */
	pcr &= ~2L;
	set_pcr(pcr);
    }

    if ((*fpu = ckfpu()) != 0) { /* We have an FPU */
    	if (SysBase->AttnFlags & AFF_68882)
    		*fpu = FPU_68882;
    	else if (SysBase->AttnFlags & AFF_68881)
    		*fpu = FPU_68881;
    	else if ((*cpu == CPU_68040) || (*cpu == CPU_68060))
    		*fpu = *cpu;
    	/* should be all cases */
    }

    *mmu = *cpu;

}

    /*
     *	Determine the Amiga Model
     */

static u_long get_model(u_long chipset)
{
    u_long model = AMI_UNKNOWN;

    if (debugflag)
	Puts("Amiga model identification:\n");
    if (probe_resource("draco.resource"))
	model = AMI_DRACO;
    else {
	if (debugflag)
	    Puts("    Chipset: ");
	switch (chipset) {
	    case CS_STONEAGE:
		if (debugflag)
		    Puts("Old or unknown\n");
		goto OCS;
		break;

	    case CS_OCS:
		if (debugflag)
		    Puts("OCS\n");
OCS:		if (probe_resident("cd.device"))
		    model = AMI_CDTV;
		else
		    /* let's call it an A2000 (may be A500, A1000, A2500) */
		    model = AMI_2000;
		break;

	    case CS_ECS:
		if (debugflag)
		    Puts("ECS\n");
		if (probe_resident("Magic 36.7") ||
		    probe_resident("kickad 36.57") ||
		    probe_resident("A3000 Bonus") ||
		    probe_resident("A3000 bonus"))
		    /* let's call it an A3000 (may be A3000T) */
		    model = AMI_3000;
		else if (probe_resource("card.resource"))
		    model = AMI_600;
		else
		    /* let's call it an A2000 (may be A500[+], A1000, A2500) */
		    model = AMI_2000;
		break;

	    case CS_AGA:
		if (debugflag)
		    Puts("AGA\n");
		if (probe_resident("A1000 Bonus") ||
		    probe_resident("A4000 bonus"))
		    model = probe_resident("NCR scsi.device") ? AMI_4000T :
								AMI_4000;
		else if (probe_resource("card.resource"))
		    model = AMI_1200;
		else if (probe_resident("cd.device"))
		    model = AMI_CD32;
		else
		    model = AMI_3000PLUS;
		break;
	}
    }
    if (debugflag) {
	Puts("\nType a key to continue...");
	GetChar();
	Puts("\n\n");
    }
    return(model);
}


    /*
     *	Probe for a Resident Modules
     */

static int probe_resident(const char *name)
{
    const struct Resident *res;

    if (debugflag)
	Printf("    Module `%s': ", name);
    res = FindResident(name);
    if (debugflag) {
	if (res)
	    Printf("0x%08lx\n", (u_long)res);
	else
	    Puts("not present\n");
    }
    return(res ? TRUE : FALSE);
}


    /*
     *	Probe for an available Resource
     */

static int probe_resource(const char *name)
{
    const void *res;

    if (debugflag)
	Printf("    Resource `%s': ", name);
    res = OpenResource(name);
    if (debugflag) {
	if (res)
	    Printf("0x%08lx\n", (u_long)res);
	else
	    Puts("not present\n");
    }
    return(res ? TRUE : FALSE);
}


    /*
     *  Create the Bootinfo structure
     */

int create_machspec_bootinfo(void)
{
    int i;
    
    /* Amiga tags */
    if (!add_bi_record(BI_AMIGA_MODEL, sizeof(bi.model), &bi.model))
	return(0);
    for (i = 0; i < bi.num_autocon; i++)
	if (!add_bi_record(BI_AMIGA_AUTOCON, sizeof(bi.autocon[i]),
			    &bi.autocon[i]))
	    return(0);
    if (!add_bi_record(BI_AMIGA_CHIP_SIZE, sizeof(bi.chip_size), &bi.chip_size))
	return(0);
    if (!add_bi_record(BI_AMIGA_VBLANK, sizeof(bi.vblank), &bi.vblank))
	return(0);
    if (!add_bi_record(BI_AMIGA_PSFREQ, sizeof(bi.psfreq), &bi.psfreq))
	return(0);
    if (!add_bi_record(BI_AMIGA_ECLOCK, sizeof(bi.eclock), &bi.eclock))
	return(0);
    if (!add_bi_record(BI_AMIGA_CHIPSET, sizeof(bi.chipset), &bi.chipset))
	return(0);
    if (!add_bi_record(BI_AMIGA_SERPER, sizeof(bi.serper), &bi.serper))
	return(0);
    return(1);
}

#ifdef BOOTINFO_COMPAT_1_0
void compat_create_machspec_bootinfo(void)
{
    int i;

    compat_bootinfo.bi_amiga.model = bi.model;
    compat_bootinfo.bi_amiga.num_autocon = bi.num_autocon;
    if (compat_bootinfo.bi_amiga.num_autocon > COMPAT_NUM_AUTO) {
	Printf("Warning: using only %d AutoConfig devices\n",
	       COMPAT_NUM_AUTO);
	compat_bootinfo.bi_amiga.num_autocon = COMPAT_NUM_AUTO;
    }
    for (i = 0; i < compat_bootinfo.bi_amiga.num_autocon; i++)
	compat_bootinfo.bi_amiga.autocon[i] = bi.autocon[i];
    compat_bootinfo.bi_amiga.chip_size = bi.chip_size;
    compat_bootinfo.bi_amiga.vblank = bi.vblank;
    compat_bootinfo.bi_amiga.psfreq = bi.psfreq;
    compat_bootinfo.bi_amiga.eclock = bi.eclock;
    compat_bootinfo.bi_amiga.chipset = bi.chipset;
    compat_bootinfo.bi_amiga.hw_present = 0;
}
#endif /* BOOTINFO_COMPAT_1_0 */


    /*
     *	Call the copy-and-go-code
     */

static void start_kernel(void (*startfunc)(), char *stackp, char *memptr,
			 u_long start_mem, u_long kernel_size, u_long rd_dest,
			 u_long rd_size)
{
    register void (*a0)() __asm("a0") = startfunc;
    register char *a2 __asm("a2") = stackp;
    register char *a3 __asm("a3") = memptr;
    register u_long a4 __asm("a4") = start_mem;
    register u_long d0 __asm("d0") = rd_dest;
    register u_long d1 __asm("d1") = rd_size;
    register u_long d2 __asm("d2") = kernel_size;
    register u_long d3 __asm("d3") = bi_size;

    __asm __volatile ("movel %%a2,%%sp;"
		      "jmp %%a0@"
		      : /* no outputs */
		      : "r" (a0), "r" (a2), "r" (a3), "r" (a4), "r" (d0),
			"r" (d1), "r" (d2), "r" (d3)
		      /* no return */);
    /* fake a noreturn */
    for (;;);
}


    /*
     *	This assembler code is copied to chip ram, and then executed.
     *	It copies the kernel to it's final resting place.
     *
     *	It is called with:
     *
     *	    a3 = memptr
     *	    a4 = start_mem
     *	    d0 = rd_dest
     *	    d1 = rd_size
     *	    d2 = kernel_size
     *	    d3 = bi_size
     */

asm(".text
	.align 4\n"
SYMBOL_NAME_STR(copyall) ":
				| /* copy kernel text and data */
	movel	%a3,%a0		| src = (u_long *)memptr;
	movel	%a0,%a2		| limit = (u_long *)(memptr+kernel_size);
	addl	%d2,%a2
	movel	%a4,%a1		| dest = (u_long *)start_mem;
1:	cmpl	%a0,%a2
	jeq	2f		| while (src < limit)
	moveb	%a0@+,%a1@+	|  *dest++ = *src++;
	jra	1b
2:
				| /* copy bootinfo to end of bss */
	movel	%a3,%a0		| src = (u_long *)(memptr+kernel_size);
	addl	%d2,%a0		| dest = end of bss (already in a1)
	movel	%d3,%d7		| count = bi_size
	subql	#1,%d7
1:	moveb	%a0@+,%a1@+	| while (--count > -1)
	dbra	%d7,1b		|     *dest++ = *src++

				| /* copy the ramdisk to the top of memory */
	movel	%a3,%a0		| src = (u_long *)(memptr+kernel_size+bi_size);
	addl	%d2,%a0
	addl	%d3,%a0
	movel	%d0,%a1		| dest = (u_long *)rd_dest;
	movel	%a0,%a2		| limit = (u_long *)(memptr+kernel_size+
	addl	%d1,%a2		|		     bi_size+rd_size);
1:	cmpl	%a0,%a2
	jeq	2f		| while (src > limit)
	moveb	%a0@+,%a1@+	|     *dest++ = *src++;
	jra	1b
2:
				| /* jump to start of kernel */
	movel	%a4,%a0		| jump_to (start_mem);
	jmp	%a0@
"
SYMBOL_NAME_STR(copyallend) ":
");


    /*
     *	Test for a MapROMmed A3640 Board
     */

asm(".text
	.align 4\n"
SYMBOL_NAME_STR(maprommed) ":
	oriw	#0x0700,%sr
	moveml	#0x3f20,%sp@-
				| /* Save cache settings */
	.long	0x4e7a1002	| movec cacr,d1 */
				| /* Save MMU settings */
	.long	0x4e7a2003	| movec tc,d2
	.long	0x4e7a3004	| movec itt0,d3
	.long	0x4e7a4005	| movec itt1,d4
	.long	0x4e7a5006	| movec dtt0,d5
	.long	0x4e7a6007	| movec dtt1,d6
	moveq	#0,%d0
	movel	%d0,%a2
				| /* Disable caches */
	.long	0x4e7b0002	| movec d0,cacr
				| /* Disable MMU */
	.long	0x4e7b0003	| movec d0,tc
	.long	0x4e7b0004	| movec d0,itt0
	.long	0x4e7b0005	| movec d0,itt1
	.long	0x4e7b0006	| movec d0,dtt0
	.long	0x4e7b0007	| movec d0,dtt1
	lea	0x07f80000,%a0
	lea	0x00f80000,%a1
	movel	%a0@,%d7
	cmpl	%a1@,%d7
	jne	1f
	movel	%d7,%d0
	notl	%d0
	movel	%d0,%a0@
	nop			| /* Thanks to Jörg Mayer! */
	cmpl	%a1@,%d0
	jne	1f
	moveq	#-1,%d0		| /* MapROMmed A3640 present */
	movel	%d0,%a2
1:	movel	%d7,%a0@
				| /* Restore MMU settings */
	.long	0x4e7b2003	| movec d2,tc
	.long	0x4e7b3004	| movec d3,itt0
	.long	0x4e7b4005	| movec d4,itt1
	.long	0x4e7b5006	| movec d5,dtt0
	.long	0x4e7b6007	| movec d6,dtt1
				| /* Restore cache settings */
	.long	0x4e7b1002	| movec d1,cacr
	movel	%a2,%d0
	moveml	%sp@+,#0x04fc
	rte
");


    /*
     *	Reset functions for nasty Zorro boards
     */

static void reset_rb3(const struct ConfigDev *cd)
{
    volatile u_char *rb3_reg = (u_char *)(cd->cd_BoardAddr+0x01002000);

    /* FN: If a Rainbow III board is present, reset it to disable */
    /* its (possibly activated) vertical blank interrupts as the */
    /* kernel is not yet prepared to handle them (level 6). */

    /* set RESET bit in special function register */
    *rb3_reg = 0x01;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit */
    *rb3_reg = 0x00;
}

static void reset_piccolo(const struct ConfigDev *cd)
{
    volatile u_char *piccolo_reg = (u_char *)(cd->cd_BoardAddr+0x8000);

    /* FN: the same stuff as above, for the Piccolo board. */
    /* this also has the side effect of resetting the board's */
    /* output selection logic to use the Amiga's display in single */
    /* monitor systems - which is currently what we want. */

    /* set RESET bit in special function register */
    *piccolo_reg = 0x01;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit */
    *piccolo_reg = 0x51;
}

static void reset_sd64(const struct ConfigDev *cd)
{
    volatile u_char *sd64_reg = (u_char *)(cd->cd_BoardAddr+0x8000);

    /* FN: the same stuff as above, for the SD64 board. */
    /* just as on the Piccolo, this also resets the monitor switch */

    /* set RESET bit in special function register */
    *sd64_reg = 0x1f;
    /* actually, only a few cycles delay are required... */
    Sleep(1000000);
    /* clear reset bit AND switch monitor bit (0x20) */
    *sd64_reg = 0x4f;
}

static void reset_ariadne(const struct ConfigDev *cd)
{
    volatile u_short *lance_rdp = (u_short *)(cd->cd_BoardAddr+0x0370);
    volatile u_short *lance_rap = (u_short *)(cd->cd_BoardAddr+0x0372);
    volatile u_short *lance_reset = (u_short *)(cd->cd_BoardAddr+0x0374);

    volatile u_char *pit_paddr = (u_char *)(cd->cd_BoardAddr+0x1004);
    volatile u_char *pit_pbddr = (u_char *)(cd->cd_BoardAddr+0x1006);
    volatile u_char *pit_pacr = (u_char *)(cd->cd_BoardAddr+0x100b);
    volatile u_char *pit_pbcr = (u_char *)(cd->cd_BoardAddr+0x100e);
    volatile u_char *pit_psr = (u_char *)(cd->cd_BoardAddr+0x101a);

    u_short in;

    Disable();

    /*
     *	Reset the Ethernet part (Am79C960 PCnet-ISA)
     */

    in = *lance_reset;   /* Reset Chip on Read Access */
    *lance_rap = 0x0000; /* PCnet-ISA Controller Status (CSR0) */
    *lance_rdp = 0x0400; /* STOP */

    /*
     *	Reset the Parallel part (MC68230 PI/T)
     */

    *pit_pacr &= 0xfd;   /* Port A Control Register */
    *pit_pbcr &= 0xfd;   /* Port B Control Register */
    *pit_psr = 0x05;     /* Port Status Register */
    *pit_paddr = 0x00;   /* Port A Data Direction Register */
    *pit_pbddr = 0x00;   /* Port B Data Direction Register */

    Enable();
}

static void reset_hydra(const struct ConfigDev *cd)
{
    volatile u_char *nic_cr  = (u_char *)(cd->cd_BoardAddr+0xffe1);
    volatile u_char *nic_isr = (u_char *)(cd->cd_BoardAddr+0xffe1 + 14);
    int n = 5000;

    Disable();
 
    *nic_cr = 0x21;	/* nic command register: software reset etc. */
    while (((*nic_isr & 0x80) == 0) && --n)  /* wait for reset to complete */
	;
 
    Enable();
}

#if 0
static void reset_a2060(const struct ConfigDev *cd)
{
#error reset_a2060: not yet implemented
}
#endif

