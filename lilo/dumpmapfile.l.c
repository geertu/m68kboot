/*
 *  Linux/m68k Loader
 *
 *  Dump the contents of a map file (for debugging)
 *
 *  (C) Copyright 1996-98 by
 *      Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
 *  and Roman Hodek (Roman.Hodek@informatik.uni-erlangen.de)
 *
 *  --------------------------------------------------------------------------
 *
 *  Usage:
 *
 *      dumpmapfile [-v|--verbose] [<mapfile>]
 *
 *  With:
 *
 *      <mapfile>   : path of the map file (default: `/boot/map')
 *
 *  --------------------------------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 */


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>

#include "config.h"

/* special tag type */
#define TAGTYPE_RAW	999

/* Dummy definition for symbols references from tagdef.c */
char *BootOptions __attribute__((unused));
char *BootRecords __attribute__((unused));
char *Files __attribute__((unused));
char *MountPointList __attribute__((unused));

#include "tagdef.c"
#include "tagnames.c"

/* table and tag section structure for tags without a section */
TAGTAB SpecialTags_table[] = {
    { TAG_LILO, TAGTYPE_RAW, 0, 0 },
    { TAG_ZEROHOLE, TAGTYPE_RAW, 0, 0 },
    { TAG_EOF, TAGTYPE_RAW, 0, 0 },
    { (u_long)-1, 0, 0, 0 }
};

TAGSECT SpecialTagSect = {
    "none",
    NULL, -1, 999999, 999999, SpecialTags_table, 0
};

/* dummy TAGTAB for start/end tags without own meaning */
TAGTAB DummyTagEnt;

/* additional tag names */
struct TagName MoreTagNames[] = {
    { TAG_LILO, "TAG_LILO" },
    { TAG_ZEROHOLE, "TAG_ZEROHOLE" },
    { TAG_EOF, "TAG_EOF" }
};

static const char *ProgramName;

static u_char *MapData;
static u_long MapSize, MapOffset = 0;


void __attribute__ ((noreturn, format (printf, 1, 2))) Die(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}


void __attribute__ ((noreturn)) Usage(void)
{
    Die("Usage: %s [-v|--verbose] [mapfile]\n", ProgramName);
}

    /*
     *	Get the Name for a Tag
     */

const char *GetTagName(u_long tag)
{
    u_int i;

    for (i = 0; i < arraysize(TagNames); i++)
	if (TagNames[i].Tag == tag)
	    return(TagNames[i].Name);
    for (i = 0; i < arraysize(MoreTagNames); i++)
	if (MoreTagNames[i].Tag == tag)
	    return(MoreTagNames[i].Name);
    return(NULL);
}

   /*
    * Find tag section started by 'tag'
    */

TAGTAB *FindTagDescr( u_long tag, TAGSECT **ts )
{
    int i;
    TAGTAB *p;

    /* first look for tags without a section */
    for( p = SpecialTags_table; p->Tag != (u_long)-1; ++p ) {
	if (p->Tag == tag) {
	    if (ts)
		*ts = &SpecialTagSect;
	    return( p );
	}
    }
    
    
    /* loop over tag sections */
    for( i = 0; i < arraysize(TagSections); ++i ) {
	/* loop over tags in that section */
	for( p = TagSections[i].tag_table; p->Tag; ++p ) {
	    if (p->Tag == tag) {
		if (ts)
		    *ts = &TagSections[i];
		return( p );
	    }
	}
	if (tag == TagSections[i].start_tag || tag == TagSections[i].end_tag) {
	    if (ts)
		*ts = &TagSections[i];
	    return( &DummyTagEnt );
	}
    }
    return( NULL );
}

    /*
     *	Get the Next Tag from the Map Data
     */

const struct TagRecord *GetNextTag(void)
{
    static const struct TagRecord *tr = NULL;

    if (tr)
	MapOffset += sizeof(struct TagRecord)+tr->Size;
    if (MapOffset+sizeof(struct TagRecord) > MapSize)
	Die("Tag %lu extends beyond end of file\n", tr->Tag);
    tr = (struct TagRecord *)&MapData[MapOffset];
    return(tr);
}



int main(int argc, char *argv[])
{
    int fh;
    const struct TagRecord *tr;
    const char *mapfile = NULL;
    int verbose = 0;
    TAGTAB *tag_ent;
    TAGSECT *tag_sect;
    const char *tag_name;

    ProgramName = argv[0];
    argc--;
    argv++;

    while (argc) {
	if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
	    Usage();
	else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose"))
	    verbose = 1;
	else if (!mapfile)
	    mapfile = argv[0];
	else
	    Usage();
	argc--;
	argv++;
    }
    if (!mapfile)
	mapfile = LILO_MAPFILE;

    if ((fh = open(mapfile, O_RDONLY)) == -1)
	Die("open %s: %s\n", mapfile, strerror(errno));
    if ((MapSize = lseek(fh, 0, SEEK_END)) == -1)
	Die("lseek %s: %s\n", mapfile, strerror(errno));
    if (!(MapData = malloc(MapSize)))
	Die("No memory\n");
    if (lseek(fh, 0, SEEK_SET) == -1)
	Die("lseek %s: %s\n", mapfile, strerror(errno));
    if (read(fh, MapData, MapSize) != MapSize)
	Die("read %s: %s\n", mapfile, strerror(errno));
    close(fh);

    do {
	tr = GetNextTag();
	if (verbose)
	    printf("0x%08lx: tag = %lu, size = %lu\n\t", MapOffset,
		   tr->Tag, tr->Size);

	if (!(tag_ent = FindTagDescr( tr->Tag, &tag_sect ))) {
	    printf( "<UNKNOWN TAG #%lu>\n", tr->Tag );
	    continue;
	}
	if (!(tag_name = GetTagName( tr->Tag )))
	    /* shoudn't happen */
	    tag_name = "<NO NAME>";

	if (tr->Tag == tag_sect->start_tag) {
	    printf( "[Start of a %s section]\n", tag_sect->name );
	    if (tag_ent == &DummyTagEnt)
		continue;
	    if (verbose)
		printf( "\t" );
	}
	if (tr->Tag == tag_sect->end_tag) {
	    printf( "[End of a %s section]\n", tag_sect->name );
	    if (tag_ent == &DummyTagEnt)
		continue;
	    if (verbose)
		printf( "\t" );
	}
	
	printf( "%s: ", tag_name );
	if (tr->Size == 0)
	    printf( "no data\n" );
	else if (tr->Tag == TAG_VECTOR) {
	    const struct vecent *vector = (const struct vecent *)tr->Data;
	    int i;
	    /* special case */
	    printf( "file size %lu bytes, device %u, entries:\n",
		    vector[0].start, vector[0].length );
	    for( i = 1; vector[i].length; i++ ) {
		printf( "\tat %8lu %6u sectors\n",
			vector[i].start, vector[i].length );
	    }
	}
	else switch( tag_ent->Type ) {
	  case TAGTYPE_INT:
	  case TAGTYPE_ARRAY:
	    printf( "0x%08lx\n", *(u_long *)tr->Data );
	    break;
	
	  case TAGTYPE_STR:
	  case TAGTYPE_CARRAY:
	    printf( "`%s'\n", (char *)tr->Data );
	    break;
	    
	  case TAGTYPE_RAW:
	    printf( "%lu bytes of raw data\n", tr->Size );
	    break;
	}
    } while( tr->Tag != TAG_EOF );
    
    if (MapOffset+sizeof(struct TagRecord)+tr->Size < MapSize)
	printf("Warning: Left %ld bytes\n", MapSize-MapOffset);
    return(0);
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
