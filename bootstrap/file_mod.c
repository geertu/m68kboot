/*
 * file_mod.c -- Trivial module for reading simple, plain files
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: file_mod.c,v 1.3 1998-02-19 19:44:09 rnhodek Exp $
 * 
 * $Log: file_mod.c,v $
 * Revision 1.3  1998-02-19 19:44:09  rnhodek
 * Integrated changes from ataboot 3.0 to 3.2
 *
 * Revision 1.2  1997/07/18 12:30:47  rnhodek
 * Moved file_mod.c from common/ to bootstrap/
 *
 * Revision 1.2  1997/07/18 11:07:08  rnhodek
 * Added sfilesize() call & Co. to streams
 *
 * Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "bootstrap.h"
#include "stream.h"

/***************************** Prototypes *****************************/

static int file_open( const char *name );
static long file_fillbuf( void *buf );
static int file_skip( long cnt );
static int file_close( void );
static long file_filesize( void );

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
	file_filesize,
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
	return( lseek( fd, cnt, SEEK_CUR ) );
}

static int file_close( void )
{
	return( close( fd ) );
}

static long file_filesize( void )
{
	long pos, size;

	pos = lseek( fd, 0, SEEK_CUR );
	if ((size = lseek( fd, 0, SEEK_END )) == -1)
		Printf( "seek to end of file failed: %s\n", strerror(errno) );
	lseek( fd, pos, SEEK_SET );
	return( size );
}


/* Local Variables: */
/* tab-width: 4     */
/* End:             */
