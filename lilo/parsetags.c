/*
 *  parsetags.c -- parse tags passed to Linux/m68k lilo
 *
 *  © Copyright 1995 by Geert Uytterhoeven, Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parsetags.c,v 1.4 1998-04-06 01:40:56 dorchain Exp $
 * 
 * $Log: parsetags.c,v $
 * Revision 1.4  1998-04-06 01:40:56  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.3  1998/02/26 11:16:13  rnhodek
 * Print notice that map file is to be readed now, so that the stream's
 * progress report doesn't look so misplaced :-)
 *
 * Revision 1.2  1998/02/26 10:08:52  rnhodek
 * Implement new TAGTYPE_CARRAY; plain ARRAY didn't work correctly for
 * string arrays.
 * FindFileVector() shouldn't return NULL for files with "bootp:" prefix,
 * otherwise they would look unavailable.
 *
 * Revision 1.1  1997/08/12 15:26:58  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdio.h>
#include <sys/types.h>

#include "strlib.h"
#include "bootstrap.h"
#include "linuxboot.h"
#include "loader.h"
#include "config.h"
#include "parsetags.h"
#include "stream.h"

/* hook for mach-dep initialization of debugging output */
extern void MachInitDebug( void );

/***************************** Prototypes *****************************/

static u_long GetDebugTag( void);
static const char *GetTagName( u_long tag);
static const struct TagRecord *GetNextTag( void);
static void UnexpectedTag( const struct TagRecord *tr);
static int EqualStrings( const char *s1, const char *s2);
static const struct TagRecord *ParseTagSection( const struct TagRecord *tr,
                                                TAGTAB *tag_table, void
                                                *optrec, u_long endtag );

/************************* End of Prototypes **************************/

extern MODULE file_mod;

#include "tagdef.c"
#include "tagnames.c"

    /*
     *	Get the Value of the Debug Tag (if it exists)
     */

static u_long GetDebugTag(void)
{
    const struct TagRecord *tr;
    u_long offset;
    int debug = 0;

    for (offset = 0; offset+sizeof(struct TagRecord)+sizeof(u_long) <= MapSize;
    	 offset += sizeof(struct TagRecord)+tr->Size) {
	tr = (struct TagRecord *)&MapData[offset];
	if (tr->Tag == TAG_DEBUG) {
	    debug = tr->Data[0];
	    break;
	}
    }
    return(debug);
}

    /*
     *	Get the Name for a Tag
     */

static const char *GetTagName(u_long tag)
{
    u_int i;

    for (i = 0; i < arraysize(TagNames); i++)
	if (TagNames[i].Tag == tag)
	    return(TagNames[i].Name);
    return(NULL);
}

   /*
    * Find tag section started by 'tag'
    */

static TAGSECT *FindTagSection( u_long tag )
{
    int i;
    
    for( i = 0; i < arraysize(TagSections); ++i )
	if (TagSections[i].start_tag == tag)
	    return( &TagSections[i] );
    return( NULL );
}

    /*
     *	Get the Next Tag from the Map Data
     */

static const struct TagRecord *GetNextTag(void)
{
    static const struct TagRecord *tr = NULL;

    if (tr)
	MapOffset += sizeof(struct TagRecord)+tr->Size;
    if (MapOffset+sizeof(struct TagRecord) > MapSize)
	Alert(AN_LILO|AO_LiloMap);
    tr = (struct TagRecord *)&MapData[MapOffset];
    return(tr);
}


    /*
     *	Unexpected Tag
     */

static void UnexpectedTag(const struct TagRecord *tr)
{
    const char *name = GetTagName(tr->Tag);

    if (name)
	Printf("Unexpected tag `%s' at offset 0x%08lx\n", name, MapOffset);
    else
	Printf("Unknown tag 0x%08lx at offset 0x%08lx\n", tr->Tag, MapOffset);
}


    /*
     *	Check whether two strings exist and are equal
     */

static int EqualStrings(const char *s1, const char *s2)
{
    return(s1 && s2 && !strcmp(s1, s2));
}


void ParseTags( void )
{
    const struct TagRecord *tr;
    struct FileDef mapfile = { NULL, "__map__", MapVectorData.Vector };
    TAGSECT *ts;
    char *rec;

    /* temporarily add the map file to the file list */
    Files = &mapfile;

    /* load map file */
    Printf( "Reading map file:\n" );
    stream_init();
    stream_push( &file_mod );
    sopen( "__map__" );
    MapSize = sfilesize();
    if (!(MapData = (u_char *)Alloc(MapSize)))
	Alert(AN_LILO|AG_NoMemory);
    sread( MapData, MapSize );
    sclose();

    /* remove map file again */
    Files = NULL;
    
    if ((Debug = GetDebugTag()))
	MachInitDebug();
	
    /* First tag must be file identification */
    tr = GetNextTag();
    if (tr->Tag != TAG_LILO)
	Alert(AN_LILO|AO_LiloMap);
    tr = GetNextTag();
    if (tr->Tag != TAG_ZEROHOLE)
	Alert(AN_LILO|AO_LiloMap);

    while(1) {
	tr = GetNextTag();
	/* look for section started by this tag */
	if (!(ts = FindTagSection( tr->Tag )))
	    break;
	if (ts->link_offset == -1 && *ts->anchor)
	    /* if no list, then error if already defined */
	    Alert(AN_LILO|AO_LiloMap);

	if (!(rec = (char *)Alloc(ts->rec_size)))
	    Alert(AN_LILO|AG_NoMemory);
	memset( rec, 0, ts->rec_size );

	if (Debug)
	    Printf( "  [%s]\n", ts->name );
	tr = ParseTagSection( tr, ts->tag_table, rec, ts->end_tag );

	*ts->anchor = rec;
	if (ts->link_offset != -1) {
	    char **nextp = (char **)(rec + ts->link_offset);
	    *nextp = NULL;
	    ts->anchor = nextp;
	}
    }
    if (tr->Tag != TAG_EOF)
	UnexpectedTag(tr);
}


#define BOENT(type,offset) (*(const type *)((char *)optrec + (offset)))

static const struct TagRecord *ParseTagSection(
    const struct TagRecord *tr, TAGTAB *tag_table, void *optrec,
    u_long endtag )
{
    TAGTAB *p;
    u_long starttag = tr->Tag;
    int i;
    
    for( ; tr->Tag != endtag; tr = GetNextTag() ) {
	for( p = tag_table; p->Tag; ++p )
	    if (tr->Tag == p->Tag)
		break;
	if (!p->Tag && tr->Tag != starttag) {
	    UnexpectedTag(tr);
	    continue;
	}
	    
	switch( p->Type ) {
	  case TAGTYPE_INT:
	    BOENT(u_long *,p->Offset) = (const u_long *)tr->Data;
	    if (Debug)
		Printf( "    %s -> %lu\n", GetTagName(tr->Tag),
			*(u_long *)tr->Data );
	    break;
	  case TAGTYPE_STR:
	    BOENT(char *,p->Offset) = (const char *)tr->Data;
	    if (Debug)
		Printf( "    %s -> \"%s\"\n", GetTagName(tr->Tag),
			(char *)tr->Data );
	    break;
	  case TAGTYPE_ARRAY:
	  case TAGTYPE_CARRAY:
	    for (i = 0; i < p->Extra; i++)
		if (!BOENT(u_long *,p->Offset+i*sizeof(u_long*)))
		    break;
	    if (i < p->Extra) {
		if (p->Type == TAGTYPE_ARRAY) {
		    BOENT(u_long *,p->Offset+i*sizeof(u_long*))
			= (const u_long *)tr->Data;
		    if (Debug)
			Printf( "    %s[%d] -> %lu\n", GetTagName(tr->Tag),
				i, *(u_long *)tr->Data );
		}
		else {
		    BOENT(char *,p->Offset+i*sizeof(char*))
			= (const char *)tr->Data;
		    if (Debug)
			Printf( "    %s[%d] -> \"%s\"\n", GetTagName(tr->Tag),
				i, (char *)tr->Data );
		}
	    }
	    else
		UnexpectedTag(tr);
	    break;
	}
    }
    return( tr );
}

    /*
     *	Free the Map Data
     */

void FreeMapData(void)
{
    struct BootRecord *record, *next;

    for (record = BootRecords; record; record = next) {
	next = record->Next;
	Free(record);
    }
    if (BootOptions)
	Free(BootOptions);
    if (MapData)
	Free(MapData);
}


    /*
     *	Find a Boot Record by Name
     */

const struct BootRecord *FindBootRecord(const char *name)
{
    const struct BootRecord *record;

    if (!name)
	name = BootOptions->Default;
    for (record = BootRecords; record; record = record->Next)
	if (EqualStrings(name, record->Label) ||
	    EqualStrings(name, record->Alias))
	    break;
    return(record);
}


    /*
     *  Find the File Vector for a File Path
     */

const struct vecent *FindVector(const char *path)
{
    const struct FileDef *file;

#ifdef USE_BOOTP
    if (strncmp( path, "bootp:", 6 ) == 0)
	/* just something != NULL */
	return( (const struct vecent *)1 );
#endif
    for (file = Files; file; file = file->Next)
	if (EqualStrings(path, file->Path))
	    break;
    return(file ? file->Vector : NULL);
}




/* Local Variables: */
/* tab-width: 8     */
/* End:             */
