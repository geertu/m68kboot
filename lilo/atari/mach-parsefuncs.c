/*
 *  Atari Linux/m68k Loader -- Configuration File Parser Utility Functions
 *
 *  © Copyright 1997 by Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: mach-parsefuncs.c,v 1.5 1998-03-10 10:26:21 rnhodek Exp $
 * 
 * $Log: mach-parsefuncs.c,v $
 * Revision 1.5  1998-03-10 10:26:21  rnhodek
 * New boot record option "restricted".
 *
 * Revision 1.4  1998/03/06 09:50:10  rnhodek
 * New option skip-on-keys, and a function to parse it.
 *
 * Revision 1.3  1998/02/26 10:34:56  rnhodek
 * New config vars WorkDir, Environ, and BootDrv (global)
 *
 * Revision 1.2  1997/09/19 09:06:59  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:10  rnhodek
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
	  | hdropts g_execprog						\
	  | hdropts g_bootdrv						\
	  | hdropts setenv							\
	  | hdropts skiponkeys

#define MACH_BOOTOPTS							\
	  | bootopts type							\
	  | bootopts restricted						\
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


static int AddTmpMnt( int dev, unsigned long start, unsigned int drive,
					   unsigned int rw_flag )
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


static struct modif {
	char *name;
	u_long mask;
} modifiers[] = {
	{ "rshift",   0x01 },
	{ "lshift",   0x02 },
	{ "shift",    0x03 },
	{ "control",  0x04 },
	{ "alt",      0x08 },
	{ "capslock", 0x10 },
	{ NULL, 0 }
};

static int isprefix( const char *s1, const char *s2 )
{
    char c1, c2;

    while (*s1) {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
		if (!c2 || c1 != c2)
			return( 0 );
    }
    return( 1 );
}

static unsigned long ParseKeyMask( const char *str )
{
	char *str2, *p;
	u_long mask = 0;
	struct modif *mod;

	/* copy string for strdup (which modifies its arg) */
	if (!(str2 = strdup( str )))
		Error_NoMemory();

	for( p = strtok( str2, " \t" ); p; p = strtok( NULL, " \t" ) ) {
		for( mod = modifiers; mod->name; ++mod ) {
			if (isprefix( p, mod->name )) {
				mask |= mod->mask;
				break;
			}
		}
		if (!mod->name)
			conferror( "Invalid modifier name\n" );
	}

	free( str2 );
	return( mask );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
