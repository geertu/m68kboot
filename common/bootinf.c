/*
 * bootinf.c -- Utility function for creating the boot info
 *
 * Copyright (c) 1993-97 by
 *   Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
 *   Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *   Andreas Schwab <schwab@issan.informatik.uni-dortmund.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: bootinf.c,v 1.8 2000-06-04 17:14:54 dorchain Exp $
 * 
 * $Log: bootinf.c,v $
 * Revision 1.8  2000-06-04 17:14:54  dorchain
 * Fixed compile errors.
 * it still doesn't work for me
 *
 * Revision 1.7  1998/04/06 01:40:51  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.6  1997/09/19 09:06:24  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.5  1997/07/17 14:22:24  geert
 * No need to include <asm/bootinfo.h>
 *
 * Revision 1.4  1997/07/16 14:22:11  rnhodek
 * Corrected version to 2.1.13
 *
 * Revision 1.3  1997/07/16 14:00:32  rnhodek
 * Forgot to include <linux/version.h>
 *
 * Revision 1.2  1997/07/16 13:57:05  rnhodek
 * Add check for version of kernel headers
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#ifdef IN_BOOTSTRAP
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#else
#ifdef IN_LILO
#include "strlib.h"
#endif
#endif
#include <sys/types.h>

/* Check that kernel headers are sufficiently new */
#include <linux/version.h>
#if LINUX_VERSION_CODE < 0x02010d
#error You need at least 2.1.13 kernel headers to compile m68kboot
#endif

#define _LINUX_TYPES_H		/* Hack to prevent including <linux/types.h> */

#include "bootstrap.h"
#include "linuxboot.h"
#include "bootinf.h"


int check_bootinfo_version( char *memptr, unsigned int mach_type,
			    unsigned int my_version,
			    unsigned int compat_version )
{
    struct bootversion *bv = (struct bootversion *)memptr;
    unsigned long version = 0;
    int i, kernel_major, kernel_minor, boots_major, boots_minor;

    Printf( "\n" );
    if (bv->magic == BOOTINFOV_MAGIC) {
	for( i = 0; bv->machversions[i].machtype != 0; ++i ) {
	    if (bv->machversions[i].machtype == mach_type) {
		version = bv->machversions[i].version;
		break;
	    }
	}
    }
    if (!version)
	Printf("Kernel has no bootinfo version info, assuming 0.0\n");

    kernel_major = BI_VERSION_MAJOR(version);
    kernel_minor = BI_VERSION_MINOR(version);
    boots_major  = BI_VERSION_MAJOR(my_version);
    boots_minor  = BI_VERSION_MINOR(my_version);
    Printf("Bootstrap's bootinfo version: %d.%d\n",
	   boots_major, boots_minor);
    Printf("Kernel's bootinfo version   : %d.%d\n",
	   kernel_major, kernel_minor);
    
    if (kernel_major == BI_VERSION_MAJOR(my_version)) {
	if (kernel_minor > boots_minor) {
	    Printf("Warning: Bootinfo version of bootstrap and kernel "
		   "differ!\n");
	    Printf("         Certain features may not work.\n");
	}
    }
#ifdef BOOTINFO_COMPAT_1_0
    else if (kernel_major == BI_VERSION_MAJOR(compat_version)) {
	Printf("(using backwards compatibility mode)\n");
    }
#endif /* BOOTINFO_COMPAT_1_0 */
    else {
	Printf("\nThis bootstrap is too %s for this kernel!\n",
	       boots_major < kernel_major ? "old" : "new");
	return 0;
    }
    return kernel_major;
}

    /*
     *  Create the Bootinfo Structure
     */

int create_bootinfo(void)
{
    int i;
    struct bi_record *record;

    /* Initialization */
    bi_size = 0;

    /* Generic tags */
    if (!add_bi_record(BI_MACHTYPE, sizeof(bi.machtype), &bi.machtype))
	return(0);
    if (!add_bi_record(BI_CPUTYPE, sizeof(bi.cputype), &bi.cputype))
	return(0);
    if (!add_bi_record(BI_FPUTYPE, sizeof(bi.fputype), &bi.fputype))
	return(0);
    if (!add_bi_record(BI_MMUTYPE, sizeof(bi.mmutype), &bi.mmutype))
	return(0);
    for (i = 0; i < bi.num_memory; i++)
	if (!add_bi_record(BI_MEMCHUNK, sizeof(bi.memory[i]), &bi.memory[i]))
	    return(0);
    if (bi.ramdisk.size)
	if (!add_bi_record(BI_RAMDISK, sizeof(bi.ramdisk), &bi.ramdisk))
	    return(0);
    if (!add_bi_string(BI_COMMAND_LINE, bi.command_line))
	return(0);

    if (!create_machspec_bootinfo())
	return(0);

    /* Trailer */
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = BI_LAST;
    bi_size += sizeof(bi_union.record.tag);

    return(1);
}

    /*
     *  Add a Record to the Bootinfo Structure
     */

int add_bi_record(u_short tag, u_short size, const void *data)
{
    struct bi_record *record;
    u_short size2;

    size2 = (sizeof(struct bi_record)+size+3)&-4;
    if (bi_size+size2+sizeof(bi_union.record.tag) > MAX_BI_SIZE) {
	Printf ("Can't add bootinfo record. Ask a wizard to enlarge me.\n");
	return(0);
    }
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = tag;
    record->size = size2;
    memcpy(record->data, data, size);
    bi_size += size2;
    return(1);
}

    /*
     *  Add a String Record to the Bootinfo Structure
     */

int add_bi_string(u_short tag, const u_char *s)
{
    return add_bi_record(tag, strlen(s)+1, (void *)s);
}


#ifdef BOOTINFO_COMPAT_1_0

    /*
     *  Create the Bootinfo structure for backwards compatibility mode
     */

int create_compat_bootinfo(void)
{
    u_int i;

    compat_bootinfo.machtype = bi.machtype;
    if (bi.cputype & CPU_68020)
	compat_bootinfo.cputype = COMPAT_CPU_68020;
    else if (bi.cputype & CPU_68030)
	compat_bootinfo.cputype = COMPAT_CPU_68030;
    else if (bi.cputype & CPU_68040)
	compat_bootinfo.cputype = COMPAT_CPU_68040;
    else if (bi.cputype & CPU_68060)
	compat_bootinfo.cputype = COMPAT_CPU_68060;
    else {
	Printf("CPU type 0x%08lx not supported by kernel\n", bi.cputype);
	return(0);
    }
    if (bi.fputype & FPU_68881)
	compat_bootinfo.cputype |= COMPAT_FPU_68881;
    else if (bi.fputype & FPU_68882)
	compat_bootinfo.cputype |= COMPAT_FPU_68882;
    else if (bi.fputype & FPU_68040)
	compat_bootinfo.cputype |= COMPAT_FPU_68040;
    else if (bi.fputype & FPU_68060)
	compat_bootinfo.cputype |= COMPAT_FPU_68060;
    else if (bi.fputype) {
	Printf("FPU type 0x%08lx not supported by kernel\n", bi.fputype);
	return(0);
    }
    compat_bootinfo.num_memory = bi.num_memory;
    if (compat_bootinfo.num_memory > COMPAT_NUM_MEMINFO) {
	Printf("Warning: using only %d blocks of memory\n",
	       COMPAT_NUM_MEMINFO);
	compat_bootinfo.num_memory = COMPAT_NUM_MEMINFO;
    }
    for (i = 0; i < compat_bootinfo.num_memory; i++) {
	compat_bootinfo.memory[i].addr = bi.memory[i].addr;
	compat_bootinfo.memory[i].size = bi.memory[i].size;
    }
    if (bi.ramdisk.size) {
	bi.ramdisk.addr &= 0xfffffc00;
	compat_bootinfo.ramdisk_size = (bi.ramdisk.size+1023)/1024;
	compat_bootinfo.ramdisk_addr = bi.ramdisk.addr;
    } else {
	compat_bootinfo.ramdisk_size = 0;
	compat_bootinfo.ramdisk_addr = 0;
    }
    strncpy(compat_bootinfo.command_line, bi.command_line, COMPAT_CL_SIZE);
    compat_bootinfo.command_line[COMPAT_CL_SIZE-1] = '\0';

    compat_create_machspec_bootinfo();
    return(1);
}
#endif /* BOOTINFO_COMPAT_1_0 */

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
