/*
 *  Atari Linux/m68k Loader -- Configuration File Parser Utility Functions
 *
 *  © Copyright 1997 by Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: mach-parsefuncs.c,v 1.1 1997-08-12 15:27:10 rnhodek Exp $
 * 
 * $Log: mach-parsefuncs.c,v $
 * Revision 1.1  1997-08-12 15:27:10  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

@include <string.h>
@include <unistd.h>
@include <errno.h>
@include <sys/stat.h>
@include "lilo.h"
@include "lilo_util.h"

#define MACH_HDROPTS							\
	  | hdropts delay							\
	  | hdropts nogui							\
	  | hdropts serial							\
	  | hdropts nobing							\
	  | hdropts videores						\
	  | hdropts mch_cookie						\
	  | hdropts cpu_cookie						\
	  | hdropts fpu_cookie						\
	  | hdropts g_tmpmnt						\
	  | hdropts g_execprog

#define MACH_BOOTOPTS							\
	  | bootopts type							\
	  | bootopts ignorettram					\
	  | bootopts loadtostram					\
	  | bootopts forcestram						\
	  | bootopts forcettram						\
	  | bootopts extramem						\
	  | bootopts tmpmnt							\
	  | bootopts execprog						\
	  | bootopts tosdriver						\
	  | bootopts bootdrv						\
	  | bootopts bootpart

#define EXCLUSIVE_ATTR(needed,os,osstr,attr,attrstr)						  \
	do {																	  \
		if (!BootRecord.OSType && BootRecord.attr)							  \
			BootRecord.OSType = CopyLong(os);								  \
		if (BootRecord.OSType) {											  \
			if (*BootRecord.OSType != os && BootRecord.attr)				  \
				conferror( attrstr " specification can be used only "		  \
						   "with OS type " osstr "\n"  );					  \
			else if (needed && *BootRecord.OSType == os && !BootRecord.attr)  \
				conferror( "Need " attrstr " specification" );				  \
		}																	  \
	} while(0)


static struct {
	int speed;
	int code;
} speedcodes[] = {
	{ 19200,  0 },
	{  9600,  1 },
	{  4800,  2 },
	{  3600,  3 },
	{  2400,  4 },
	{  2000,  5 },
	{  1800,  6 },
	{  1200,  7 },
	{   600,  8 },
	{   300,  9 },
	{   200, 10 },
	{   150, 11 },
	{   134, 12 },
	{   110, 13 },
	{    75, 14 },
	{    50, 15 }
};

static int SerialSpeedCode( int speed )
{
	int i;

	for( i = 0; i < arraysize(speedcodes); ++i ) {
		if (speedcodes[i].speed == speed)
			return( speedcodes[i].code );
	}
	return( -1 );
}


static int AddTmpMnt( int dev, unsigned long start, unsigned drive,
					   unsigned rw_flag )
{
	int i;
    struct TagTmpMnt **p, *new;

    for( i = 0, p = &Config.Mounts; *p; p = &(*p)->next, ++i )
		if (*(*p)->device    == dev &&
			*(*p)->start_sec == start &&
			*(*p)->drive     == drive &&
			*(*p)->rw        == rw_flag)
			/* already have an identical entry */
			return( i );

    if (!(new = malloc(sizeof(struct TagTmpMnt))))
		Error_NoMemory();
	new->next = NULL;
    *p = new;

    new->device = CopyLong(dev);
    new->start_sec = CopyLong(start);
    new->drive = CopyLong(drive);
    new->rw = CopyLong(rw_flag);
    return( i );
}
	
/* Local Variables: */
/* tab-width: 4     */
/* End:             */
