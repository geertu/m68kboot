/*
 *  writetags.c -- write tags to map file
 *
 *  © Copyright 1995-97 by Geert Uytterhoeven, Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: writetags.l.c,v 1.7 1998-03-04 09:11:07 rnhodek Exp $
 * 
 * $Log: writetags.l.c,v $
 * Revision 1.7  1998-03-04 09:11:07  rnhodek
 * Size of array elements for TAGTYPE_ARRAY is sizeof(u_long*), not
 * sizeof(u_long) (though it's the same...)
 *
 * Revision 1.6  1998/02/26 10:09:48  rnhodek
 * Implement new TAGTYPE_CARRAY; plain ARRAY didn't work correctly for
 * string arrays.
 * If filename has a "bootp:" prefix, don't create a TAG_VECTOR for it.
 *
 * Revision 1.5  1998/02/24 11:13:38  rnhodek
 * In WriteTags(), print filename passed as argument instead of constant.
 * In WriteZeroTag(), don't modify file position, otherwise TAG_LILO is
 * overwritten.
 *
 * Revision 1.4  1998/02/19 20:40:13  rnhodek
 * Make things compile again
 *
 * Revision 1.3  1997/09/19 09:06:49  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.2  1997/08/12 21:51:04  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:26:59  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#define _LINUX_STAT_H		/* Hack to prevent including <linux/stat.h> */
struct inode;			/* to avoid warning */
#include <linux/fs.h>

#include "config.h"
#include "lilo_util.h"
#include "parser.h"
#include "writetags.h"


/***************************** Prototypes *****************************/

static void WriteTagSection( int fd, TAGSECT *ts, void *rec );
static void WriteTagData( int fd, u_long tag, const void *data, u_long
                          size);
static void WriteTagString( int fd, u_long tag, const char *s);
static void WriteZeroTag( int fd, const char *fname );

/************************* End of Prototypes **************************/


#define FROM_WRITETAGS
#include "tagdef.c"

void WriteTags( const char *fname )
{
	TAGSECT *ts;
	int i, fd;
	char *p;
    const struct BootRecord *record, *defrecord = NULL;

	/* Print out boot records */
	puts( "\nBoot records:" );
	for( record = Config.Records; record; record = record->Next) {
		printf( "    %s", record->Label );
		if (EqualStrings(Config.Options.Default, record->Label) ||
			EqualStrings(Config.Options.Default, record->Alias)) {
			defrecord = record;
			printf( " (default)" );
		}
		printf( "\n" );
	}

    if (Verbose)
		printf("Creating map file `%s'\n", fname);
    if ((fd = open(fname, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
		Error_Open(fname);
	
    /* File Identification */
    WriteTagData(fd, TAG_LILO, NULL, 0);
	/* Zero Sectors for Mapping Holes */
	WriteZeroTag(fd, fname);

	for( i = 0; i < arraysize(TagSections); ++i ) {
		ts = &TagSections[i];
		if (Verbose)
			printf("  [ %s ]\n", ts->name );

		if (ts->link_offset == -1)
			/* single structure */
			WriteTagSection( fd, ts, (void *)ts->anchor );
		else
			/* list of structures */
			for( p = *ts->anchor; p; p = *(char **)(p+ts->link_offset) )
				WriteTagSection( fd, ts, p );
	}
	
    /* End of File */
    WriteTagData(fd, TAG_EOF, NULL, 0);
    close(fd);

    if (!Verbose)
		puts("");
}


static void WriteTagSection( int fd, TAGSECT *ts, void *rec )
{
	TAGTAB *tag_table = ts->tag_table;
    TAGTAB *p;
	const char *q;
	int i;
	
	/* if start tag isn't mentioned in the tag table (where it must be first),
	 * write it with a null value */
	if (tag_table->Tag != ts->start_tag)
		WriteTagData( fd, ts->start_tag, NULL, 0 );

	for( p = tag_table; p->Tag; ++p ) {
		/* special case for TAG_VECTOR */
		if (p->Tag == TAG_VECTOR) {
			struct FileDef *file = (struct FileDef *)rec;
			const char *path = CreateFileName(file->Path);
			int vecsize, numblocks;
#ifdef USE_BOOTP
			/* don't create a TAG_VECTOR for remote files */
			if (strncmp( path, "bootp:", 6 ) != 0) {
#endif
				vecsize = 0;
				if ((file->Vector = CreateVector(path, &numblocks)))
					vecsize = (numblocks+1)*sizeof(struct vecent);
				else
					printf("Warning: file `%s' doesn't exist\n", path);
				if (Verbose && file->Vector)
					printf("    %s (%ld bytes)\n", path,file->Vector[0].start);
				WriteTagData( fd, p->Tag, file->Vector, vecsize );
#ifdef USE_BOOTP
			}
#endif
		}
		else switch( p->Type ) {
		  case TAGTYPE_INT:
			q = *(const char **)(rec + p->Offset);
			if (q)
				WriteTagData( fd, p->Tag, q, sizeof(u_long) );
			break;
		  case TAGTYPE_STR:
			q = *(const char **)(rec + p->Offset);
			if (q)
				WriteTagString( fd, p->Tag, q );
			break;
		  case TAGTYPE_ARRAY:
			for( i = 0; i < p->Extra; ++i ) {
				q = *(const char **)(rec + p->Offset + i*sizeof(u_long*));
				if (q)
					WriteTagData( fd, p->Tag, q, sizeof(u_long) );
			}
			break;
		  case TAGTYPE_CARRAY:
			for( i = 0; i < p->Extra; ++i ) {
				q = *(const char **)(rec + p->Offset + i*sizeof(char*));
				if (q)
					WriteTagString( fd, p->Tag, q );
			}
			break;
		}
	}

	WriteTagData( fd, ts->end_tag, NULL, 0 );
}


    /*
     *	Write a Tag Record to the Map File
     */

static void WriteTagData(int fd, u_long tag, const void *data, u_long size)
{
    struct TagRecord tr;

    tr.Tag = tag;
    tr.Size = size;
    if (write(fd, &tr, sizeof(tr)) != sizeof(tr))
		Error_Write(MapFile);
    if (size) {
		if (write(fd, data, size) != size)
			Error_Write(MapFile);
    }
}


static void WriteTagString(int fd, u_long tag, const char *s)
{
    WriteTagData(fd, tag, s, (strlen(s)+4)&~3);
}


/*
 * Write enough zeros so that MAX_HOLE_SECTORS are filled up. These
 * sectors are needed for mapping holes in files.
 */
static void WriteZeroTag( int fd, const char *fname )
{
	u_long blksize, zerosize, offset, start;
	unsigned int sectors_per_block;
	off_t pos;
	char *zeros;
	dev_t dev;
	
    if (ioctl(fd, FIGETBSZ, &blksize) == -1)
		Error_Ioctl(fname, "FIGETBSZ");
    sectors_per_block = blksize/HARD_SECTOR_SIZE;

	if ((pos = lseek( fd, 0, SEEK_CUR )) == -1)
		Error_Seek(fname);
	/* The first blksize-pos bytes fill up the current block that also
	 * contains TAG_LILO; the next blksize ones are really used for mapping
	 * the holes. */
	zerosize = 2*blksize - pos;
	if (!(zeros = malloc( zerosize )))
		Error_NoMemory();
	memset( zeros, 0, zerosize );
	WriteTagData(fd, TAG_ZEROHOLE, zeros, zerosize );
	free( zeros );

	/* translate second block of file */
	offset = 1;
	if (ioctl(fd, FIBMAP, &offset) == -1)
	    Error_Ioctl(fname, "FIBMAP");
	GeometryFile( fname, &dev, &start );

	MaxHoleSectors = sectors_per_block;
	HoleSector = start + offset * sectors_per_block;
}	


/* Local Variables: */
/* tab-width: 4     */
/* End:             */
