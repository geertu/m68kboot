/*
 * stream.c -- Implementation of data streams for Atari bootstrapper
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * $Id: stream.c,v 1.9 1998-04-09 08:05:17 rnhodek Exp $
 * 
 * $Log: stream.c,v $
 * Revision 1.9  1998-04-09 08:05:17  rnhodek
 * Use ELF_LOADER for whether to use int_fkt_offset_jmp().
 *
 * Revision 1.8  1998/04/07 09:55:36  rnhodek
 * Change logic which set of MOD_* macros is to be used:
 * int_fkt_offset_jmp is only for Amiga Lilo.
 *
 * Revision 1.7  1998/04/06 01:40:52  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.6  1998/02/26 10:04:10  rnhodek
 * Reset fpos and last_shown on stream_push(), so that their counts don't
 * show up during later opens.
 *
 * Revision 1.5  1998/02/24 11:11:41  rnhodek
 * In stream_push(), set mod->down->up only if downstream module exists
 *
 * Revision 1.4  1997/07/18 11:07:08  rnhodek
 * Added sfilesize() call & Co. to streams
 *
 * Revision 1.3  1997/07/17 14:21:57  geert
 * Fix typos in comments
 *
 * Revision 1.2  1997/07/16 15:06:24  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
 * Import sources into CVS
 *
 * 
 *
 * The streams implemented in this file are intended to clearly organize the
 * data sources and transformations used by bootstrap to read the kernel and
 * ramdisk image. They also free the single modules from managing data
 * buffers, minimizes the memory needed for buffers in the various stages and
 * the amount of copying between them.
 *
 * Terminology: A stream consists of a stack of modules. All of those either
 * "produce" data (e.g. read them from disk or receive them via TFTP), or do
 * transformations on data (e.g. decompress them). Producing modules must be
 * at the bottom of the stack, and they don't call modules below them. Their
 * order in the stack represents a preference which method to use to get the
 * data. Transforming modules are above the producing modules and need the
 * modules below to get the data they work on. They can call sopen(), sread()
 * and the other stream interface functions just the usual way.
 *
 * Interface functions:
 *
 *   stream_init(): Initialize the stream by removing all modules.
 *
 *   stream_push(mod): Pushes a new module MOD onto the stack. All modules
 *     must be ready before any other function is called.
 *
 *   sopen(name): Open the stream, NAME is the name of a file or some other
 *     entity to access.
 *
 *   sfilesize(): Return size of file, or -1 if the size cannot be determined
 *     (e.g. BOOTP/TFTP, gunzip). Should be called only immediately after
 *     sopen().
 *
 *   sread(buf,cnt): Read data from the stream, just like the Unix read()
 *     function. Returns number of bytes written to BUF. This is lower than
 *     CNT only at EOF. -1 means some error.
 *
 *   sseek(whence,offset): Seek to some other location in the byte stream,
 *     arguments are as with Unix lseek(). Seeking backwards is supported only
 *     to some unspecified border, but small steps back should work after
 *     reading a small amount of data. SEEK_END as WHENCE is not supported,
 *     since the size isn't always known. Return value is the new position in
 *     the stream, or < 0 for error.
 *
 *   sclose(): Close and de-init the stream.
 *
 * Module interface:
 *
 * Each module has to supply a struct of type MODULE describing itself. The
 * struct consists of a name, a maximum buffer size, and four module methods.
 * The max. buffer size is the biggest number of bytes a call to fillbuf() can
 * return. If this is actually unlimited for the module, use some reasonable
 * value that doesn't make reading inefficient, but also doesn't waste memory.
 * 32k seems ok.
 *
 *   open(name): Open the file (or other entity) NAME. Transforming modules
 *     usually pass this request down, and may do additional internal
 *     initializations. Producing modules check whether they can supply data,
 *     and then grab the stream tail. Otherwise, they deregister (retval 1).
 *     Return value is 0 for OK, 1 for "remove me from the stream please, I
 *     can't do anything", and < 0 for some error. 1 for transforming modules
 *     means that the transformation isn't to be applied (e.g. the file isn't
 *     compressed). If going to return 1, the open method must call sopen()
 *     for the modules downstreams itself, and return 0 or -1 according to
 *     success of this. This allows modules to open the downstream channel,
 *     check it, and if the data seen are not applicable just return. If the
 *     upper layer would do the opening, it couldn't tell whether the stream
 *     below the current module is already open or not.
 *
 *   fillbuf(buf): Fill the buffer BUF with data. This should not write more
 *     than maxbuf bytes, but it can write less. It returns the number of
 *     bytes returned, or < 0 for an error.
 *
 *   skip(cnt): Skip CNT bytes of the stream. This method is optional and may
 *     be NULL if the module can't implement it reasonably. (E.g., on
 *     decompressing it's impossible to skip, the data in between have to be
 *     decompressed anyway.) The new position in the stream is returned
 *     (this may be less than requested). A return value < 0 stands for error.
 *
 *   close(): Close this module and do any deinitializations necessary. Return
 *     0 for ok, < 0 for error.
 *
 *   filesize(): Return size of file. This method is optional and if not
 *     present, sfilesize() will return -1.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <minmax.h>
#include <linux/linkage.h>

#include "bootstrap.h"
#include "stream.h"

/* definition of the dummy head module */
MODULE head_mod = {
	"head",						/* name */
	0,							/* maxbuf (unused) */
	NULL, NULL, NULL, NULL, NULL,		/* methods */
	MOD_REST_INIT
};

static int stream_dont_display = 0;


/***************************** Prototypes *****************************/

static void stream_show_progress( void );

/************************* End of Prototypes **************************/


/* ------------------------------------------------------------------------ */
/*								Initialization								*/

/* initialize the module stack */
void stream_init( void )
{
	currmod = &head_mod;
	head_mod.up = head_mod.down = NULL;
}

/* push a module onto the stream
 * 
 * The new module is inserted after the head module, i.e. on top of the other
 * modules registered before.
 */
void stream_push( MODULE *mod )
{
	mod->down = head_mod.down;
	mod->up = &head_mod;
	head_mod.down = mod;
	if (mod->down)
		mod->down->up = mod;
	mod->fpos = mod->last_shown = 0;
}

/* ------------------------------------------------------------------------ */
/*									Macros									*/

/* go up and down the stream */										\
#define DOWN_MOD()													\
	do {															\
		if (!(currmod = currmod->down)) {							\
	    	Printf( "Internal error: bottom-most module %s calls "	\
					"downstreams!\n", currmod->name );				\
		    exit( 1 );												\
		}															\
	} while(0)

#define UP_MOD()												\
	do {														\
		if (!(currmod = currmod->up)) {							\
			Printf( "Internal error: topmost module %s calls "	\
					"upstreams!\n", currmod->name );			\
			exit( 1 );											\
		}														\
    } while(0)

#ifdef ELF_LOADER
/* need to adjust addresses from function tables */
extern u_long int_fkt_offset_jmp(void *,...);
#define MOD_OPEN(name)		(int_fkt_offset_jmp(currmod->open, (name)))
#define MOD_FILLBUF(buf)	(int_fkt_offset_jmp(currmod->fillbuf, (buf) ))
#define MOD_SKIP(off)		(int_fkt_offset_jmp(currmod->skip, (off) ))
#define MOD_CLOSE()			(int_fkt_offset_jmp(currmod->close))
#define	MOD_FILESIZE()		(int_fkt_offset_jmp(currmod->filesize))
#else
/* macros for accessing the methods of current module */
#define MOD_OPEN(name)		((*currmod->open)( (name) ))
#define MOD_FILLBUF(buf)	((*currmod->fillbuf)( (buf) ))
#define MOD_SKIP(off)		((*currmod->skip)( (off) ))
#define MOD_CLOSE()			((*currmod->close)())
#define	MOD_FILESIZE()		((*currmod->filesize)())
#endif

#define ADJUST_USERBUF(len)						\
	do {										\
		buf += (len);							\
		cnt -= (len);							\
		currmod->fpos += (len);					\
	} while(0)

#define ADJUST_MODBUF(len)						\
	do {										\
		currmod->bufp += (len);					\
		currmod->buf_cnt -= (len);				\
	} while(0)

#define TEST_ERR(e)	do { if ((e)<0) { rv = (e); goto err_out; } } while(0)
#define TEST_EOF(e) do { if ((e)==0) { currmod->eof = 1; goto out; } } while(0)

/* ------------------------------------------------------------------------ */
/*								  Functions									*/
		
/* open the stream */
int sopen( const char *name )
{
	int rv;
	
	stream_dont_display++;
	DOWN_MOD();
	rv = MOD_OPEN( name );
	if (rv > 0) {
		/* remove module from the stream */
		if (currmod->down) {
			currmod->up->down = currmod->down;
			currmod->down->up = currmod->up;
			currmod = currmod->down;
		}
		else
			/* Was the bottom-most module, i.e. no module feels responsible
			 * for producing data -> no data available :-( */
			rv = -1;
	}
	else if (rv == 0) {
		/* init buffering data */
		currmod->fpos       =
		currmod->buf_cnt    =
		currmod->eof        = 0;
		currmod->last_shown = -1;
		if (!(currmod->buf = Alloc( currmod->maxbuf ))) {
			Printf( "Out of buffer memory for module %s\n",
					currmod->name );
			exit( 1 );
		}
		currmod->bufp = currmod->buf;
	}
	UP_MOD();
	stream_dont_display--;
	return( rv );
}

long sfilesize( void )
{
	long rv;
	
	DOWN_MOD();
	rv = currmod->filesize ? MOD_FILESIZE() : -1;
	UP_MOD();
	return( rv );
}

long sread( void *buf, long cnt )
{
	long len, rv;
	void *bufstart = buf;
	
	DOWN_MOD();

	if (currmod->eof) {
		return( 0 );
	}
	
	if (currmod->buf_cnt) {
		/* take data from buffer as far as possible */
		len = min( currmod->buf_cnt, cnt );
		memcpy( buf, currmod->bufp, len );
		ADJUST_USERBUF(len);
		ADJUST_MODBUF(len);
		stream_show_progress();
	}

	while( cnt >= currmod->maxbuf ) {
		/* while fillbuf chunks fit into user buffer, call fillbuf for there
		 * directly */
		len = MOD_FILLBUF( buf );
		TEST_ERR(len);
		TEST_EOF(len);
		ADJUST_USERBUF(len);
		stream_show_progress();
	}

	while( cnt ) {
		/* rest of request must be buffered */
		currmod->buf_cnt = MOD_FILLBUF( currmod->buf );
		currmod->bufp = currmod->buf;
		TEST_ERR( currmod->buf_cnt );
		TEST_EOF( currmod->buf_cnt );

		len = min( currmod->buf_cnt, cnt );
		memcpy( buf, currmod->buf, len );
		ADJUST_USERBUF(len);
		ADJUST_MODBUF(len);
		stream_show_progress();
	}

  out:
	rv = buf - bufstart;
  err_out:
	UP_MOD();
	return( rv );
}

#define RETURN(v)	do { rv = (v); goto err_out; } while(0)
	
int sseek( long offset, int whence )
{
	int rv;
	long newpos, len;
	
	DOWN_MOD();

	switch( whence ) {
	  case SEEK_SET:
		newpos = offset;
		break;
	  case SEEK_CUR:
		newpos = currmod->fpos + offset;
		break;
	  case SEEK_END:
	  default:
		/* not supported */
		Printf( "Unsupported seek operation for module %s\n",
				currmod->name );
		RETURN( -1 );
	}

	if (newpos == currmod->fpos)
		goto out;
	
	if (newpos < currmod->fpos) {
		/* backward seeks are only supported inside the current buffer */
		long bufstartpos = currmod->fpos - (currmod->bufp - currmod->buf);
		long back;
		if (!currmod->buf_cnt || newpos < bufstartpos) {
			Printf( "Unsupported backward seek in module %s "
					"(bufstart=%ld, dstpos=%ld)\n",
					currmod->name,
					currmod->buf_cnt ? bufstartpos : -1,
					newpos );
			RETURN( -1 );
		}
		back = currmod->fpos - newpos;
		currmod->bufp -= back;
		currmod->buf_cnt += back;
		currmod->fpos = newpos;
		goto out;
	}

	if (currmod->buf_cnt && newpos <= currmod->fpos + currmod->buf_cnt) {
		/* seek is forward inside current buffer */
		long fwd = newpos - currmod->fpos;
		ADJUST_MODBUF( fwd );
		currmod->fpos += fwd;
		goto out;
	}

	/* otherwise: always need to advance buffer (if present) */
	if (currmod->buf_cnt) {
		currmod->fpos += currmod->buf_cnt;
		currmod->buf_cnt = 0;
	}

	/* let the module skip, if it can */
	if (currmod->skip) {
		len = MOD_SKIP( newpos - currmod->fpos );
		TEST_ERR( len );
		currmod->fpos = len;
	}

	/* otherwise, read and junk the data */
	while( currmod->fpos < newpos ) {
		/* rest of request must be buffered */
		currmod->buf_cnt = MOD_FILLBUF( currmod->buf );
		currmod->bufp = currmod->buf;
		TEST_ERR( currmod->buf_cnt );
		TEST_EOF( currmod->buf_cnt );

		len = min( currmod->buf_cnt, newpos-currmod->fpos );
		ADJUST_MODBUF(len);
		currmod->fpos += len;
	}
	stream_show_progress();

  out:
	rv = currmod->fpos;
  err_out:
	UP_MOD();
	return( rv );
}

int sclose( void )
{
	int rv;

	stream_show_progress();
	Printf( "\n" );
	stream_dont_display++;
	DOWN_MOD();

	rv = MOD_CLOSE();
	Free( currmod->buf );

	UP_MOD();
	stream_dont_display--;
	return( rv );
}


#define SHOW_SHIFT	13

static void stream_show_progress( void )
{
	static char rotchar[4] = { '|', '/', '-', '\\' };
	MODULE *p;
	int notnull = 0;
	long spos = currmod->fpos >> SHOW_SHIFT;
	
	if (stream_dont_display || spos == currmod->last_shown)
		return;
	currmod->last_shown = spos;

	Printf( "\r" );
	for( p = head_mod.down; p; p = p->down ) {
		if (!p->fpos && notnull)
			break;
		if (p->fpos)
			notnull = 1;
		Printf( " %c %7ld %-8s ",
				rotchar[(p->fpos / max(p->maxbuf,1<<SHOW_SHIFT)) & 3],
				p->fpos, p->name );
	}
#ifdef atarist
	fflush( stdout );
#endif
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
