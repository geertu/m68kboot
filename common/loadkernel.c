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
 * $Id: loadkernel.c,v 1.6 1998-04-06 01:40:52 dorchain Exp $
 * 
 * $Log: loadkernel.c,v $
 * Revision 1.6  1998-04-06 01:40:52  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.5  1997/07/18 12:10:33  rnhodek
 * Call open_ramdisk only if ramdisk_name set; 0 return value means error.
 * Rename load_ramdisk/move_ramdisk to open_ramdisk/load_ramdisk, in parallel
 * to the *_kernel functions.
 * Rewrite open/load_ramdisk so that the temp storage and additional memcpy
 * are avoided if file size known after sopen().
 *
 * Revision 1.4  1997/07/16 17:31:16  rnhodek
 * Replaced SERROR with ERROR, sclose() now done in cleanup()
 *
 * Revision 1.3  1997/07/16 15:06:23  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.2  1997/07/16 14:03:35  rnhodek
 * On errors, clean up and return 0 instead of exit
 *
 * Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
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

/* linux specific include files */
#include <asm/types.h>
#define _LINUX_TYPES_H		/* Hack to prevent including <linux/types.h> */
#include <linux/a.out.h>
#include <linux/elf.h>
#include <asm/page.h>

#include "bootstrap.h"
#include "loadkernel.h"
#include "bootstrap.h"
#include "stream.h"


/* declarations of stream modules */
extern MODULE file_mod;
extern MODULE gunzip_mod;
#ifdef USE_BOOTP
extern MODULE bootp_mod;
#endif

/* to make error handling shorter... */
#define ERROR(fmt,rest...)			\
    do {					\
	Printf( fmt, ##rest );			\
	cleanup();				\
	return( 0 );				\
    } while(0)


static int stream_open = 0;
    
/* header data of kernel executable (ELF) */
static Elf32_Ehdr kexec_elf;
static Elf32_Phdr *kernel_phdrs = NULL;

#ifdef AOUT_KERNEL
/* header data of kernel executable (a.out) */
static struct exec kexec;
static int elf_kernel;
static u_long text_offset = 0;
#else
#define elf_kernel 1
#endif

#define RD_CHUNK_SIZE	(128*1024)
static char **rdptr = NULL;
static int n_rdptrs = 0;


/***************************** Prototypes *****************************/

static void cleanup( void );

/************************* End of Prototypes **************************/



unsigned long open_kernel( const char *kernel_name )
{
    int i;
    unsigned long kernel_size;
    
    stream_init();
    stream_push( &file_mod );
#ifdef USE_BOOTP
    stream_push( &bootp_mod );
#endif
    stream_push( &gunzip_mod );
    
    if (sopen( kernel_name ) < 0)
	ERROR( "Unable to get kernel image %s\n", kernel_name );
    stream_open = 1;

    if (sread( &kexec_elf, sizeof(kexec_elf) ) != sizeof(kexec_elf))
	ERROR( "Cannot read ELF header of kernel image\n" );

    if (memcmp( &kexec_elf.e_ident[EI_MAG0], ELFMAG, SELFMAG ) == 0) {
#ifdef AOUT_KERNEL
	elf_kernel = 1;
#endif
	if (kexec_elf.e_type != ET_EXEC || kexec_elf.e_machine != EM_68K ||
	    kexec_elf.e_version != EV_CURRENT)
	    ERROR( "Invalid ELF header contents in kernel\n" );

	/* Load the program headers */
	kernel_phdrs = (Elf32_Phdr *)Alloc( kexec_elf.e_phnum *
					    sizeof (Elf32_Phdr) );
	if (!kernel_phdrs)
	    ERROR( "Unable to allocate memory for program headers\n" );
	sseek( kexec_elf.e_phoff, SEEK_SET );
	if (sread( kernel_phdrs, kexec_elf.e_phnum * sizeof (*kernel_phdrs) )
	    != kexec_elf.e_phnum * sizeof (*kernel_phdrs))
	    ERROR( "Unable to read program headers from %s\n", kernel_name );
    }
    else {
#ifdef AOUT_KERNEL
	/* try to interprete as a.out kernel */
	if (sread( &kexec, sizeof(kexec) ) != sizeof(kexec))
	    ERROR( "Unable to read exec header from %s\n", kernel_name );
	switch (N_MAGIC(kexec)) {
	  case ZMAGIC:
	    text_offset = N_TXTOFF(kexec);
	    break;
	  case QMAGIC:
	    text_offset = sizeof(kexec);
	    /* the text size includes the exec header; remove this */
	    kexec.a_text -= sizeof(kexec);
	    break;
	  default:
	    ERROR( "Wrong magic number %lo in kernel header\n",
		    N_MAGIC(kexec) );
	}
	elf_kernel = 0;
#else
	ERROR( "Kernel image is no ELF executable\n" );
#endif
    }

    /* Align bss size to multiple of four */
#ifdef AOUT_KERNEL
    if (!elf_kernel)
      kexec.a_bss = (kexec.a_bss + 3) & ~3;
#endif
	
    /* calculate the total required amount of memory */
    if (elf_kernel) {
	u_long min_addr = 0xffffffff, max_addr = 0;
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    if (min_addr > kernel_phdrs[i].p_vaddr)
	      min_addr = kernel_phdrs[i].p_vaddr;
	    if (max_addr < kernel_phdrs[i].p_vaddr + kernel_phdrs[i].p_memsz)
	      max_addr = kernel_phdrs[i].p_vaddr + kernel_phdrs[i].p_memsz;
	}
	/* This is needed for newer linkers that include the header in
	   the first segment.  */
	if (min_addr == 0) {
	    min_addr = PAGE_SIZE;
	    kernel_phdrs[0].p_vaddr += PAGE_SIZE;
	    kernel_phdrs[0].p_offset += PAGE_SIZE;
	    kernel_phdrs[0].p_filesz -= PAGE_SIZE;
	    kernel_phdrs[0].p_memsz -= PAGE_SIZE;
	}
	kernel_size = max_addr - min_addr;
    }
#ifdef AOUT_KERNEL
    else
	kernel_size = kexec.a_text + kexec.a_data + kexec.a_bss;
#endif

    return( kernel_size );
}

int load_kernel( void *memptr )
{
    int i;
    
    /* read the text and data segments from the kernel image */
    if (elf_kernel) {
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    if (sseek( kernel_phdrs[i].p_offset, SEEK_SET) == -1)
		ERROR( "Failed to seek to segment %d\n", i );
	    if (sread( memptr + kernel_phdrs[i].p_vaddr - PAGE_SIZE,
		       kernel_phdrs[i].p_filesz )
		!= kernel_phdrs[i].p_filesz)
		ERROR( "Failed to read segment %d\n", i );
	}
    }
#ifdef AOUT_KERNEL
    else {
	if (sseek( text_offset, SEEK_SET) == -1)
	    ERROR( "Failed to seek to text segment\n" );
	if (sread( memptr, kexec.a_text) != kexec.a_text)
	    ERROR( "Failed to read text segment\n" );
	/* data follows immediately after text */
	if (sread( memptr + kexec.a_text, kexec.a_data) != kexec.a_data)
	    ERROR( "Failed to read data segment\n" );
    }
#endif
    sclose();
    stream_open = 0;
    return( 1 );
}

void kernel_debug_infos( unsigned long base )
{
    int i;
    
    if (elf_kernel) {
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    Printf ("Kernel segment %d at %#lx, size %d\n", i,
		    base + kernel_phdrs[i].p_vaddr - PAGE_SIZE,
		    kernel_phdrs[i].p_memsz);
	}
    }
#ifdef AOUT_KERNEL
    else {
	Printf ("\nKernel text at %#lx, code size %d\n",
		start_mem, kexec.a_text);
	Printf ("Kernel data at %#lx, data size %d\n",
		start_mem + kexec.a_text, kexec.a_data );
	Printf ("Kernel bss  at %#lx, bss  size %d\n",
		start_mem + kexec.a_text + kexec.a_data, kexec.a_bss );
    }
#endif
}


/*
 * open and init the ramdisk
 *
 * This must be done before creating the bootinfo, because for that the
 * ramdisk size is needed. Unfortunately, that size can't be determined in all
 * cases (if it goes through gunzip_mod or bootp_mod). Therefore
 * open_ramdisk() has two strategies: If the file size can be determined with
 * sfilesize(), it just opens the rd and returns its size. load_ramdisk() will
 * physically load the data.
 *
 * On the other hand, if we have no file size, we have to load the ramdisk
 * completely already here to a temp storage. We can't reuse that loading
 * area, because the rd image must be *after* the kernel image, the mover code
 * relies on this fact to avoid overwriting data. So load_ramdisk() will copy
 * the ramdisk data from the temp area to its final place. (For allocation
 * that, the size is needed...) I can't see a really nice solution for this
 * :-( I've choosen the scheme below because it is faster than using realloc()
 * to dynamically extend the load area (avoids potentially many copy
 * actions).
 */
unsigned long open_ramdisk( const char *ramdisk_name )
{
    int n;
    unsigned long rd_size;

    /* init a new stream stack, and omit gunzip_mod, the kernel can
     * decompress the ramdisk itself */
    stream_init();
    stream_push( &file_mod );
#ifdef USE_BOOTP
    stream_push( &bootp_mod );
#endif
    
    if (sopen( ramdisk_name ) < 0)
	ERROR( "Unable to open ramdisk file %s\n", ramdisk_name );
    stream_open = 1;

    if ((rd_size = sfilesize()) != -1) {
	/* have file size, return immediately; must close stream, only one at
	 * one time possible */
	sclose();
	stream_open = 0;
	return( rd_size );
    }

    /* alternative strategy if file size unknown: load to temp area */

    rd_size = 0;
    do {
	rdptr = ReAlloc( rdptr, n_rdptrs*sizeof(char *),
			        (n_rdptrs+1)*sizeof(char *) );
	if (!rdptr || !(rdptr[n_rdptrs] = Alloc( RD_CHUNK_SIZE )))
	    ERROR( "Out of memory for ramdisk image\n" );
	++n_rdptrs;
	
	n = sread( rdptr[n_rdptrs-1], RD_CHUNK_SIZE );
	if (n < 0)
	    ERROR( "Error while reading ramdisk image\n" );
	if (n == 0) {
	    Free( rdptr[n_rdptrs-1]);
	    break;
	}
	rd_size += n;
    } while( n == RD_CHUNK_SIZE );
    sclose();
    stream_open = 0;

    return( rd_size );
}


int load_ramdisk( const char *ramdisk_name, void *dst, unsigned long rd_size )
{
    char **srcp = rdptr;
    unsigned long left = rd_size;

    /* return if no ramdisk used */
    if (!rd_size)
	return( 1 );
    
    if (!rdptr) {
	/* ramdisk not yet loaded, open_ramdisk just determined size */

	/* init a new stream stack, and omit gunzip_mod, the kernel can
	 * decompress the ramdisk itself */
	stream_init();
	stream_push( &file_mod );
#ifdef USE_BOOTP
	stream_push( &bootp_mod );
#endif
	if (sopen( ramdisk_name ) < 0)
	    ERROR( "Unable to open ramdisk file %s\n", ramdisk_name );
	stream_open = 1;
	if (sread( dst, rd_size ) != rd_size)
	    ERROR( "Error while reading ramdisk image\n" );
	sclose();
	stream_open = 0;
	return( 1 );
    }
    
    /* if we already have loaded the ramdisk, move it from the temp area to
     * above the kernel code; the moving scheme at boot time requires the
     * ramdisk to be physically above the kernel! */

    /* keep the non-constant-length part out of this loop, the memcpy can
     * be better optimized then */
    for( ; left > RD_CHUNK_SIZE;
	 ++srcp, dst += RD_CHUNK_SIZE, left -= RD_CHUNK_SIZE ) {
	memcpy( dst, *srcp, RD_CHUNK_SIZE );
	Free( *srcp );
    }
    if (left) {
	memcpy( dst, *srcp, left );
	Free( *srcp );
    }
    Free( rdptr );
    rdptr = NULL;

    return( 1 );
}


static void cleanup( void )
{
    int i;

    if (stream_open)
	sclose();
    
    if (kernel_phdrs) {
	Free( kernel_phdrs );
	kernel_phdrs = NULL;
    }

    if (rdptr) {
	for( i = 0; i < n_rdptrs; ++i )
	    Free( rdptr[i] );
	Free( rdptr );
    }
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
