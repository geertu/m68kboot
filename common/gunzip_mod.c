/*
 * gunzip_mod.c -- Decompressing module for Atari bootstrap
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: gunzip_mod.c,v 1.7 1998-04-09 08:04:41 rnhodek Exp $
 * 
 * $Log: gunzip_mod.c,v $
 * Revision 1.7  1998-04-09 08:04:41  rnhodek
 * Use ELF_LOADER for switching SYMBOL_NAME_STR
 *
 * Revision 1.6  1998/04/07 10:05:17  rnhodek
 * Change logic for which SYMBOL_NAME_STR should be used: without '_' is
 * only for Amiga Lilo.
 *
 * Revision 1.5  1998/04/06 01:40:51  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.4  1997/09/19 09:06:38  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.3  1997/07/18 11:07:08  rnhodek
 * Added sfilesize() call & Co. to streams
 *
 * Revision 1.2  1997/07/16 15:06:23  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#ifdef ELF_LOADER
#include "strlib.h"
#include <linux/linkage.h>
#else
#include <stdlib.h>
#include <string.h>
#undef SYMBOL_NAME_STR
#define SYMBOL_NAME_STR(X) "_"#X
#endif
#include <unistd.h>

#include "bootstrap.h"
#include "bootp.h"
#include "stream.h"


/*
 * gzip declarations
 */

#define OF(args)  args

#define memzero(s, n)     memset ((s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define INBUFSIZ	(32*1024)	/* input buffer size; for optimum performance
								 * the same or a multiple of other buffer
								 * sizes, and file_mod uses 32k */
#define WSIZE		(32*1024)	/* window size--must be a power of two, and */
								/* at least 32K for zip's deflate method */
#define CHANGING_WINDOW			/* this tell inflate.c that 'window' can
								 * change in flush_window(), and that the
								 * previous window (needed to copy data) is
								 * preserved in 'previous_window' */

/***************************** Prototypes *****************************/

static int gunzip_open( const char *name );
static long gunzip_fillbuf( void *buf );
static int gunzip_close( void );
static int call_gunzip( void );
static void gzip_mark( void **ptr );
static void gzip_release( void **ptr );
static int fill_inbuf( void );
static void flush_window( void );
static void error( char *x );

/************************* End of Prototypes **************************/



MODULE gunzip_mod = {
	"gunzip",					/* name */
	WSIZE,						/* max. 512 bytes per TFTP packet */
	gunzip_open,
	gunzip_fillbuf,
	NULL,						/* cannot skip */
	gunzip_close,
	NULL,						/* can't determine size of file at open time */
	MOD_REST_INIT
};

/* special stack for gunzip() function */
static char *gunzip_stack;
/* some globals for storing values between stack switches */
static long gunzip_sp, gunzip_jumpback, main_sp, main_fp;
/* size of stack for gunzip(); 4k should be more than enough. (I guess 2k
 * would suffice also, but I don't want to track down stack overflows...) */
#define GUNZIP_STACK_SIZE	(4*1024)

static uch *inbuf;
static uch *window;
static uch *previous_window;

static unsigned int insize = 0;  /* valid bytes in inbuf */
static unsigned int inptr = 0;   /* index of next byte to be processed in inbuf */
static unsigned int outcnt = 0;  /* bytes in output buffer */
static int exit_code = 0;
static long bytes_out = 0;

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())
		
/* Diagnostic functions (stubbed out) */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

#define STATIC static


static int gunzip_open( const char *name )
{
	int rv;
	char myname[strlen(name)+4];
	unsigned char buf[2];
	
	/* try opening downstream channel, first with name passed, then with ".gz"
	 * appended */
	strcpy( myname, name );
	if (sopen( myname ) >= 0)
		goto got_it_open;
	strcat( myname, ".gz" );
	if (sopen( myname ) >= 0) 
		goto got_it_open;
	if (strcmp( name, "vmlinux" ) == 0) {
		/* if name is "vmlinux", also try "vmlinuz" */
		strcpy( myname, "vmlinuz" );
		if (sopen( myname ) >= 0)
			goto got_it_open;
	}
	return( -1 );
	
  got_it_open:
	/* check if data is gzipped */
	rv = sread( buf, 2 );
	sseek( 0, SEEK_SET );
	if (rv < 2) {
		Printf( "File shorter than 2 bytes, can't test for gzip\n" );
		return( -1 );
	}
    if (buf[0] != 037 || (buf[1] != 0213 && buf[1] != 0236))
		/* not compressed, remove this module from the stream */
		return( 1 );

	if (!(gunzip_stack = Alloc( GUNZIP_STACK_SIZE ))) {
		Printf( "Out of memory for gunzip stack!\n" );
		return( -1 );
	}
	gunzip_sp = 0;

	if (!(inbuf = Alloc( INBUFSIZ ))) {
		Printf( "Out of memory for gunzip input buffer!\n" );
		return( -1 );
	}

	Printf( "Decompressing %s\n", myname );
	return( 0 );
}

static __inline__ unsigned long getsp( void )
{
	ulg ret;
	__asm__ __volatile__
		( "movl	%/sp,%0" : "=r" (ret) : );
	return( ret );
}

static long gunzip_fillbuf( void *buf )
{
	previous_window = window;
	window = buf;
	return( call_gunzip() );
}

static int gunzip_close( void )
{
	Free( gunzip_stack );
	Free( inbuf );
	sclose();
	return( 0 );
}

#include "inflate.c"

static void gzip_mark( void **ptr )
{
}

static void gzip_release( void **ptr )
{
}

/* This function does most of the magic for implementing a second stack for
 * gunzip(). This is necessary because lib/inflate.c only provides a callback
 * (flush_window()) that can store data away. But we need to actually return
 * from out fillbuf method to implement the streaming. (Otherwise, the whole
 * file would have to be uncompressed and buffered as a whole.)
 *
 * The solution to this problem is a second stack on which gunzip() runs. If
 * flush_window() is called, it saves state and then switches back to the main
 * stack, from which we can return to our caller. For resuming at the point
 * where it left off, we simply restore the gunzip stack again. A second
 * return value in d1 distinguishes returns from inside flush_window() and
 * "normal" returns from gunzip() itself. The latter either indicate EOF or
 * error.
 */
static int call_gunzip( void )
{
	int rv = 0;
	
	/* avoid warnings about unused variables (appear only in asm) */
	(void)&main_sp;
	(void)&main_fp;
	(void)&gunzip_jumpback;
	(void)gunzip;
	
	if (!gunzip_sp) {
		register int asm_rv __asm__ ("d0");
		/* gunzip() wasn't called before: set up its stack and call the main
		 * function */
		makecrc();
		__asm__ __volatile__ (
			"movl	%/sp,"SYMBOL_NAME_STR(main_sp)"\n\t"	/* save current sp */
			"movl	%/a6,"SYMBOL_NAME_STR(main_fp)"\n\t"	/* and fp */
			"movl	%1,%/sp\n\t"		/* move to new stack */
			"jbsr	"SYMBOL_NAME_STR(gunzip)"\n\t"		/* call the function */
		SYMBOL_NAME_STR(return_from_flush)":\n\t" 		/* point of coming back */
			"movl	"SYMBOL_NAME_STR(main_sp)",%/sp\n\t"	/* restore main sp */
			"movl	"SYMBOL_NAME_STR(main_fp)",%/a6" 		/* and fp */
			: "=d" (asm_rv)
			: "g" (gunzip_stack+GUNZIP_STACK_SIZE-sizeof(int))
			: "d1", "d2", "d3", "d4", "d5", "d6", "d7", "a0", "a1", "a2", "a3",
			  "a4", "a5", "memory" /* all regs possibly clobbered */
			);
		rv = asm_rv;
	}
	else {
		/* gunzip() is already active, jump to where it left off */
		__asm__ __volatile__ (
			"movl	%/sp,"SYMBOL_NAME_STR(main_sp)"\n\t"	/* save current sp */
			"movl	%/a6,"SYMBOL_NAME_STR(main_fp)"\n\t"	/* and fp */
			"movl	"SYMBOL_NAME_STR(gunzip_jumpback)",%/a0\n\t"
			"jmp	%/a0@"
			: /* no outputs */
			: /* no inputs */
			: "d0", "d1", "a0", "a1", "memory" /* clobbered regs */
			);
	}
	return( rv );		/* 0 for EOF, -1 for error, > 0 if from flush_window */
}

/* This is used in flush_window() to return into call_gunzip() */
#define RETURN_TO_MAIN_STACK()												\
	__asm__ __volatile__ (													\
		"movml	%/d2-%/d7/%/a2-%/a6,%/sp@-\n\t"	/* save call-saved regs */	\
		"movl	%/sp,"SYMBOL_NAME_STR(gunzip_sp)"\n\t" 			/* save current sp */		\
		"movl	#1f,"SYMBOL_NAME_STR(gunzip_jumpback)"\n\t" 		/* save return address */	\
		"movl	"SYMBOL_NAME_STR(outcnt)",%/d0\n\t"				/* return value */			\
		"jmp	"SYMBOL_NAME_STR(return_from_flush)"\n" 			/* and return... */			\
	"1:\tmovl	"SYMBOL_NAME_STR(gunzip_sp)",%/sp\n\t" 			/* restore sp */			\
		"movml	%/sp@+,%/d2-%/d7/%/a2-%/a6"		/* restore registers */		\
		: /* no outputs */													\
		: /* no inputs */													\
		: "d0", "d1", "a0", "a1", "memory" /* clobbered regs */				\
		)

/*
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 */
static int fill_inbuf( void )
{
    if (exit_code)
		return( -1 );

    insize = sread( inbuf, INBUFSIZ );
    if (insize <= 0)
		return( -1 );

    inptr = 1;
    return( inbuf[0] );
}

/*
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
static void flush_window( void )
{
    ulg c = crc;         /* temporary variable */
    unsigned int n;
    uch *in, ch;
    
    in = window;
    for( n = 0; n < outcnt; n++ ) {
	    ch = *in++;
	    c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
    }
    crc = c;
	bytes_out += (ulg)outcnt;

	/* return to call_gunzip(), next call to it resumes here */
	RETURN_TO_MAIN_STACK();
	
	outcnt = 0;
}

static void error( char *x )
{
    Printf( "\n%s\n", x);
    exit_code = 1;
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
