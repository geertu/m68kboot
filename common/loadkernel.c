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
 * $Id: loadkernel.c,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
 * 
 * $Log: loadkernel.c,v $
 * Revision 1.1  1997-07-15 09:45:37  rnhodek
 * Initial revision
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

/* linux specific include files */
#include <asm/types.h>
#define _LINUX_TYPES_H		/* Hack to prevent including <linux/types.h> */
#include <linux/a.out.h>
#include <linux/elf.h>
#include <asm/page.h>

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
	fprintf( stderr, fmt, ##rest );		\
	boot_exit( EXIT_FAILURE );		\
    } while(0)

#define SERROR(fmt,rest...)			\
    do {					\
	fprintf( stderr, fmt, ##rest );		\
	sclose();				\
	boot_exit( EXIT_FAILURE );		\
    } while(0)


/* header data of kernel executable (ELF) */
static Elf32_Ehdr kexec_elf;
static Elf32_Phdr *kernel_phdrs = NULL;

#ifdef AOUT_KERNEL
/* header data of kernel executable (a.out) */
struct exec kexec;
int elf_kernel;
static u_long text_offset = 0;
#else
#define elf_kernel 1
#endif



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

    if (sread( &kexec_elf, sizeof(kexec_elf) ) != sizeof(kexec_elf))
	SERROR( "Cannot read ELF header of kernel image\n" );

    if (memcmp( &kexec_elf.e_ident[EI_MAG0], ELFMAG, SELFMAG ) == 0) {
#ifdef AOUT_KERNEL
	elf_kernel = 1;
#endif
	if (kexec_elf.e_type != ET_EXEC || kexec_elf.e_machine != EM_68K ||
	    kexec_elf.e_version != EV_CURRENT)
	    SERROR( "Invalid ELF header contents in kernel\n" );

	/* Load the program headers */
	kernel_phdrs = (Elf32_Phdr *)malloc( kexec_elf.e_phnum *
					     sizeof (Elf32_Phdr) );
	if (!kernel_phdrs)
	    SERROR( "Unable to allocate memory for program headers\n" );
	sseek( kexec_elf.e_phoff, SEEK_SET );
	if (sread( kernel_phdrs, kexec_elf.e_phnum * sizeof (*kernel_phdrs) )
	    != kexec_elf.e_phnum * sizeof (*kernel_phdrs))
	    SERROR( "Unable to read program headers from %s\n", kernel_name );
    }
    else {
#ifdef AOUT_KERNEL
	/* try to interprete as a.out kernel */
	if (sread( &kexec, sizeof(kexec) ) != sizeof(kexec))
	    SERROR( "Unable to read exec header from %s\n", kernel_name );
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
	    SERROR( "Wrong magic number %lo in kernel header\n",
		    N_MAGIC(kexec) );
	}
	elf_kernel = 0;
#else
	SERROR( "Kernel image is no ELF executable\n" );
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

void load_kernel( void *memptr )
{
    int i;
    
    /* read the text and data segments from the kernel image */
    if (elf_kernel) {
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    if (sseek( kernel_phdrs[i].p_offset, SEEK_SET) == -1)
		SERROR( "Failed to seek to segment %d\n", i );
	    if (sread( memptr + kernel_phdrs[i].p_vaddr - PAGE_SIZE,
		       kernel_phdrs[i].p_filesz )
		!= kernel_phdrs[i].p_filesz)
		SERROR( "Failed to read segment %d\n", i );
	}
    }
#ifdef AOUT_KERNEL
    else {
	if (sseek( text_offset, SEEK_SET) == -1)
	    SERROR( "Failed to seek to text segment\n" );
	if (sread( memptr, kexec.a_text) != kexec.a_text)
	    SERROR( "Failed to read text segment\n" );
	/* data follows immediately after text */
	if (sread( memptr + kexec.a_text, kexec.a_data) != kexec.a_data)
	    SERROR( "Failed to read data segment\n" );
    }
#endif
    sclose();
}

void kernel_debug_infos( unsigned long base )
{
    int i;
    
    if (elf_kernel) {
	for (i = 0; i < kexec_elf.e_phnum; i++) {
	    printf ("Kernel segment %d at %#lx, size %d\n", i,
		    base + kernel_phdrs[i].p_vaddr - PAGE_SIZE,
		    kernel_phdrs[i].p_memsz);
	}
    }
#ifdef AOUT_KERNEL
    else {
	printf ("\nKernel text at %#lx, code size %d\n",
		start_mem, kexec.a_text);
	printf ("Kernel data at %#lx, data size %d\n",
		start_mem + kexec.a_text, kexec.a_data );
	printf ("Kernel bss  at %#lx, bss  size %d\n",
		start_mem + kexec.a_text + kexec.a_data, kexec.a_bss );
    }
#endif
}


#define RD_CHUNK_SIZE	(128*1024)
static char **rdptr = NULL;

unsigned long load_ramdisk( const char *ramdisk_name )
{
    int n, n_rdptrs = 0;
    unsigned long rd_size = 0;

    /*
     * load the ramdisk
     *
     * This must be done before creating the bootinfo (there the ramdisk size
     * is needed). But the ramdisk image must also reside physically *after*
     * the kernel image. (This condition is needed by the mover to avoid
     * overwriting data.) This is ensured by using only one malloc() for
     * kernel+ramdisk, but the size for that malloc() is known only after the
     * ramdisk is loaded... I can't see a really nice solution for this :-(
     * I've choosen the scheme below because it is faster than using realloc()
     * to dynamically extend the ramdisk block (avoids potentially many copy
     * actions). 
     */
    if (!ramdisk_name)
	return( 0 );

    /* init a new stream stack, and omit gunzip_mod, the kernel can
     * decompress the ramdisk itself */
    stream_init();
    stream_push( &file_mod );
#ifdef USE_BOOTP
    stream_push( &bootp_mod );
#endif
    
    if (sopen( ramdisk_name ) < 0)
	ERROR( "Unable to open ramdisk file %s\n", ramdisk_name );
	
    do {
	rdptr = realloc( rdptr, (n_rdptrs+1)*sizeof(char *) );
	if (!rdptr || !(rdptr[n_rdptrs] = malloc( RD_CHUNK_SIZE )))
	    SERROR( "Out of memory for ramdisk image\n" );
	
	n = sread( rdptr[n_rdptrs], RD_CHUNK_SIZE );
	if (n < 0)
	    SERROR( "Error while reading ramdisk image\n" );
	if (n == 0) {
	    free( rdptr[n_rdptrs]);
	    break;
	}
	rd_size += n;
	n_rdptrs++;
    } while( n == RD_CHUNK_SIZE );
    sclose();

    return( rd_size );
}


void move_ramdisk( void *dst, unsigned long rd_size )
{
    char **srcp = rdptr;
    unsigned long left = rd_size;
    
    /* if we have a ramdisk, move it above the kernel code; the moving scheme
     * at boot time requires the ramdisk to be physically above the kernel! */
    if (!rd_size)
	return;
    
    /* keep the non-constant-length part out of this loop, the memcpy can
     * be better optimized then */
    for( ; left > RD_CHUNK_SIZE;
	 ++srcp, dst += RD_CHUNK_SIZE, left -= RD_CHUNK_SIZE ) {
	memcpy( dst, *srcp, RD_CHUNK_SIZE );
	free( *srcp );
    }
    if (left) {
	memcpy( dst, *srcp, left );
	free( *srcp );
    }
    free( rdptr );
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
