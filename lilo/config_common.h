/*
 *  Amiga Linux/m68k Loader -- Configuration
 *
 *  � Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: config_common.h,v 1.1 1997-08-12 15:26:56 rnhodek Exp $
 * 
 * $Log: config_common.h,v $
 * Revision 1.1  1997-08-12 15:26:56  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _config_common_h
#define _config_common_h

#include "parsetags.h"

#define LILO_ID			(0x4C494C4F)	/* `LILO' */
#define LILO_MAPVECTORID	(0x4D415056)	/* `MAPV' */

#define LILO_CONFIGFILE		"/etc/lilo.conf"
#define LILO_MAPFILE		"/boot/map"
#define LILO_LOADERTEMPLATE	"/boot/loader"
#define LILO_LOADERFILE		"/boot/loader.patched"
#define LILO_BACKUPFILEPREFIX	"/boot/backup"


struct TagRecord {
    u_long Tag;			/* Tag Identifier */
    u_long Size;		/* Must be a multiple of 4 bytes */
    u_long Data[0];		/* Size bytes of Tag Data */
};

/* Constants for 'Type' field in TAGTAB */
#define TAGTYPE_INT	0
#define TAGTYPE_STR	1
#define TAGTYPE_ARRAY	2

typedef struct {
    u_long Tag;
    int Type;
    u_long Offset;
    u_long Extra; /* array size or function to call */
} TAGTAB;

/* end marker in tag table */
#define END_TAGTAB { 0, 0, 0, 0 }

typedef struct {
    char *name;			/* name of section */
    char **anchor;		/* address of anchor to list of records */
    int link_offset;		/* offset of link or -1 if no list */
    u_long start_tag;		/* start tag */
    u_long end_tag;		/* end tag */
    TAGTAB *tag_table;		/* tag table for parsing */
    unsigned rec_size;		/* size of one list element */
} TAGSECT;

#define TAG_LILO		LILO_ID	/* File Identification */
#define TAG_EOF			(0)	/* End of File */
#define TAG_ZEROHOLE		(1)	/* Empty Space for Mapping Holes */

    /*
     *  File Definition
     */

/* Type of vector entries */
struct vecent {
	u_long start;
	u_short length;
} __attribute((packed));


struct TagName {
    u_long Tag;
    const char *Name;
};

#include "tagdef.h"

#define MAXVECTORSIZE	(512)		/* Should be sufficient */
struct FileVectorData {
    u_long ID[2];
    u_long MaxVectorSize;
    struct vecent Vector[MAXVECTORSIZE];
};

extern const struct FileVectorData MapVectorData;

#ifndef arraysize
#define arraysize(x)	(sizeof(x)/sizeof(*(x)))
#endif

#ifndef offsetof
#define offsetof(s,f)	((u_long)(&(((s *)0)->f)))
#endif

#endif  /* _config_common_h */

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
