/*
 * stream.h -- Stream definitions
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: stream.h,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
 * 
 * $Log: stream.h,v $
 * Revision 1.1  1997-07-15 09:45:37  rnhodek
 * Initial revision
 *
 * 
 */

#ifndef _stream_h
#define _stream_h

typedef struct _module {
    /* data supplied by the module */
    char *name;							/* name of this module */
    long maxbuf;						/* max. bytes a fillbuf() call
										 * can return */
    /* methods */
    int (*open)( const char *name );	/* open the module */
    long (*fillbuf)( void *buf );		/* fill a buffer, max. maxbuf bytes
										 * returned */
    int (*skip)( long cnt );			/* skip data (for seek), optional,
										 * returns amount  skipped */
    int (*close)( void );				/* close the module */
    /* data maintained by general streams layer */
    char *buf;							/* current buffer */
    char *bufp;							/* pointer into buffer */
    long buf_cnt;						/* #chars left in buffer */
    long fpos;							/* position in stream */
    int eof;							/* EOF indicator */
	long last_shown;					/* last call of show_progress */
    /* links to neighbour modules */
    struct _module *down, *up;
} MODULE;

/* initializer for fields in MODULE not supplied by module */
#define MOD_REST_INIT								\
	NULL, NULL, 0, 0, 0, 0,	/* buffer data */		\
	NULL, NULL				/* down, up pointers */

MODULE *currmod;		/* currently active module */

/***************************** Prototypes *****************************/

void stream_init( void );
void stream_push( MODULE *mod );
int sopen( const char *name );
long sread( void *buf, long cnt );
int sseek( long offset, int whence );
int sclose( void );

/************************* End of Prototypes **************************/
    
#endif  /* _stream_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
