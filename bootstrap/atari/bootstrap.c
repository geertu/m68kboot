/*
 * bootstrap.c -- Atari bootstrapper main program
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
 * This file is the only part of the Atari bootstrapper that relies on being a
 * TOS executable. All other parts can be reused in some kind of LILO.
 *
 * History:
 * 	16 Jun 1997 Major internal rework, split into several files.
 * 
 * Old history, applying to all sources of the bootstrapper:
 *	11 Jun 1997 Fix for unpadded gzipped ramdisks with bootinfo interface
 *		    version 1.0 (Geert, Roman)
 *	01 Feb 1997 Implemented kernel decompression (Roman)
 *	28 Nov 1996 Fixed and tested previous change (James)
 *	27 Nov 1996 Compatibility with bootinfo interface version 1.0 (Geert)
 *	12 Nov 1996 Fixed and tested previous change (Andreas)
 *	18 Aug 1996 Updated for the new boot information structure (untested!)
 *		    (Geert)
 *	10 Dec 1995 BOOTP/TFTP support (Roman)
 *	03 Oct 1995 Allow kernel to be loaded to TT ram again (Andreas)
 *	11 Jul 1995 Add support for ELF format kernel (Andreas)
 *	16 Jun 1995 Adapted to Linux 1.2: kernel always loaded into ST ram
 *		    (Andreas)
 *      14 Nov 1994 YANML (Yet Another New Memory Layout :-) kernel
 *		    start address is KSTART_ADDR + PAGE_SIZE, this
 *		    does not need the ugly kludge with
 *		    -fwritable-strings (++andreas)
 *      09 Sep 1994 Adapted to the new memory layout: All the boot_info entry
 *                  mentions all ST-Ram and the mover is located somewhere
 *                  in the middle of memory (roman)
 *                  Added the default arguments file known from the other
 *                  bootstrap version
 *      19 Feb 1994 Changed everything so that it works? (rdv)
 *      14 Mar 1994 New mini-copy routine used (rdv)
 *
 * $Id: bootstrap.c,v 1.2 1997-07-30 21:41:28 rnhodek Exp $
 * 
 * $Log: bootstrap.c,v $
 * Revision 1.2  1997-07-30 21:41:28  rnhodek
 * Only remove an ugly empty line
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "linuxboot.h"
#include "version.h"

extern char *optarg;
extern int optind;


/***************************** Prototypes *****************************/

static void usage( void );
static void parse_extramem( const char *p );
static unsigned long parse_size( const char *p );
static void get_default_args( int *argc, char ***argv );

/************************* End of Prototypes **************************/


static void usage( void )
{
    fprintf( stderr,
	     "Usage:\n"
	     "\tbootstrap [-dnstST] [-k kernel_executable] "
	     "[-r ramdisk_file] [-m start:size] [kernel options...]\n");
    getchar();
    exit(EXIT_FAILURE);
}

int main( int argc, char *argv[] )
{
    int ch, cmdlen;
    
    /* print the startup message */
    puts( "\fLinux/68k Atari Bootstrap version " VERSION WITH_BOOTP );
    puts( "Copyright 1993-97 by Arjan Knor, Robert de Vries, "
	  "Roman Hodek, Andreas Schwab\n" );

    /* ++roman: If no arguments on the command line, read them from
     * file */
    if (argc == 1)
	get_default_args( &argc, &argv );

    /* check arguments */
    while ((ch = getopt(argc, argv, "dntsS:T:k:r:m:")) != EOF)
	switch (ch) {
	  case 'd':
	    debugflag = 1;
	    break;
	  case 't':
	    ignore_ttram = 1;
	    break;
	  case 'T':
	    force_tt_size = parse_size( optarg );
	    break;
	  case 's':
	    load_to_stram = 1;
	    break;
	  case 'S':
	    force_st_size = parse_size( optarg );
	    break;
	  case 'k':
	    kernel_name = optarg;
	    break;
	  case 'r':
	    ramdisk_name = optarg;
	    break;
	  case 'm':
	    parse_extramem( optarg );
	    break;
	  case 'n':
#ifdef USE_BOOTP
	    no_bootp = 1;
#endif
	    break;
	  case '?':
	  default:
	    usage();
	}

    argc -= optind;
    argv += optind;
  
    /*
     * Copy rest of command line options into the kernel command line.
     */
    cmdlen = 0;
    command_line[0] = 0;
    while (argc--) {
	if ((cmdlen+strlen(*argv)+1) < CL_SIZE-1) {
	    cmdlen += strlen(*argv) + 1;
	    if (command_line[0])
		strcat( command_line, " " );
	    strcat( command_line, *argv++ );
	}
	else {
	    printf( "Warning: Must omit parameter '%s', kernel command "
		    "line too long!\n", *argv );
	}
    }

    linux_boot();
    /* never reached */
}


static void parse_extramem( const char *p )
{
    char *end;
    
    extramem_start = strtoul( p, &end, 0 );
    if (end == p) {
	fprintf( stderr, "Invalid number for extra mem start: %s\n", p );
	usage();
    }
    if (*end != ':') {
	fprintf( stderr, "':' missing after extra mem start\n" );
	usage();
    }
    
    extramem_size = parse_size( end+1 );
}

static unsigned long parse_size( const char *p )
{
    unsigned long size;
    char *end;
    
    size = strtoul( p, &end, 0 );
    if (end == p) {
	fprintf( stderr, "Invalid number: %s\n", p );
	usage();
    }

    if (islower(*end) == 'k')
	size *= 1024;
    else if (islower(*end) == 'm')
	size *= 1024*1024;
    return( size );
}

#define	MAXARGS		30

static void get_default_args( int *argc, char ***argv )
{
    FILE *f;
    static char *nargv[MAXARGS];
    char arg[256], *p;
    int c, quote, state;

    if (!(f = fopen( "bootargs", "r" )))
	return;
	
    *argc = 1;
    if (***argv)
	nargv[0] = **argv;
    else
	nargv[0] = "bootstrap";
    *argv = nargv;

    quote = state = 0;
    p = arg;
    while( (c = fgetc(f)) != EOF ) {		

	if (state == 0) {
	    /* outside args, skip whitespace */
	    if (!isspace(c)) {
		state = 1;
		p = arg;
	    }
	}
		
	if (state) {
	    /* inside an arg: copy it into 'arg', obeying quoting */
	    if (!quote && (c == '\'' || c == '"'))
		quote = c;
	    else if (quote && c == quote)
		quote = 0;
	    else if (!quote && isspace(c)) {
		/* end of this arg */
		*p = 0;
		nargv[(*argc)++] = strdup(arg);
		state = 0;
	    }
	    else
		*p++ = c;
	}
    }
    if (state) {
	/* last arg finished by EOF! */
	*p = 0;
	nargv[(*argc)++] = strdup(arg);
    }
    fclose( f );
	
    nargv[*argc] = 0;
}    


void boot_exit( int status )
{
    /* wait for a keypress, so error messages don't disapper without a chance
     * to read them */
    getchar();

    /* go back to user mode; set usp to current sp before Super() call to keep
     * the current stack */
    __asm__ __volatile__ ( "movel sp,usp" );
    (void)Super( userstk );
    exit( status );
}



/* Local Variables: */
/* tab-width: 8     */
/* End:             */
