/*
 * file_mod.c -- Trivial module for reading simple, plain files
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: file_mod.c,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
 * 
 * $Log: file_mod.c,v $
 * Revision 1.1  1997-07-15 09:45:37  rnhodek
 * Initial revision
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "stream.h"

/***************************** Prototypes *****************************/

static int file_open( const char *name );
static long file_fillbuf( void *buf );
static int file_skip( long cnt );
static int file_close( void );

/************************* End of Prototypes **************************/

#define MAXBUF	(32*1024)

/* definition of the module structure */
MODULE file_mod = {
	"file",						/* name */
	MAXBUF,						/* maxbuf (arbitrary) */
	file_open,
	file_fillbuf,
	file_skip,
	file_close,
	MOD_REST_INIT
};

static int fd;					/* the fd for the file */

static int file_open( const char *name )
{
	/* strip off "local:" prefix, if any */
	if (strncmp( name, "local:", 6 ) == 0)
		name += 6;
	fd = open( name, O_RDONLY );
	return( fd < 0 ? -1 : 0 );
}

static long file_fillbuf( void *buf )
{
	return( read( fd, buf, MAXBUF ) );
}

static int file_skip( long cnt )
{
	return( lseek( fd, SEEK_CUR, cnt ) );
}

static int file_close( void )
{
	return( close( fd ) );
}


/* Local Variables: */
/* tab-width: 4     */
/* End:             */
