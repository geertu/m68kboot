/*
 * tmpmnt.c -- Temporary mounts for Atari Linux
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: tmpmnt.c,v 1.2 1997-09-19 09:07:00 geert Exp $
 * 
 * $Log: tmpmnt.c,v $
 * Revision 1.2  1997-09-19 09:07:00  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:13  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <osbind.h>
#include <errno.h>

#include "bootstrap.h"
#include "config.h"
#include "loader.h"
#include "sysvars.h"
#include "menu.h"
#include "tmpmnt.h"


#define MAX_DRIVES	16
static unsigned int my_drives = 0;
static struct drv_param {
	unsigned int dev;
	unsigned long start;
	unsigned long secsize;
	unsigned int mediach;
	unsigned int rw;
} drv_param[MAX_DRIVES];
static _BPB global_bpb;

struct boot_sector {
  unsigned char		boot_jump[2];	/* jump to boot code */
  unsigned char 	filler[6];		/* junk */
  unsigned char 	serial[3];		/* serial number */
  unsigned char		sector_size[2];	/* bytes per logical sector */
  unsigned char		cluster_size[1];/* sectors/cluster */
  unsigned char		rsvd_sect[2];	/* reserved sectors */
  unsigned char		nfats[1];		/* number of FATs */
  unsigned char		dir_entries[2];	/* root directory entries */
  unsigned char		sectors[2];		/* number of sectors */
  unsigned char		media[1];		/* media code (unused) */
  unsigned char		fat_length[2];	/* sectors/FAT */
  unsigned char		secs_track[2];	/* sectors per track */
  unsigned char		heads[2];		/* number of heads */
};

/* access to 1 and 2 byte fields (byteswap) */
#define BOOTSEC_FIELD1(bs,field)	((bs)->field[0])
#define BOOTSEC_FIELD2(bs,field)	((bs)->field[0] | ((bs)->field[1] << 8))

typedef struct {
	unsigned long magic;
	unsigned long id;
	unsigned long oldvec;
} XBRA;

static int VectorsInstalled = 0;


/***************************** Prototypes *****************************/

static void install_vectors( void );
static void uninstall_vectors( void );
static int remove_vector( unsigned long *vec, const char *vecname );
static long do_rwabs( int first_arg )
	__attribute__ ((unused));
	static _BPB *do_getbpb( unsigned int drv );
static long do_mediach( unsigned int _drv )
	__attribute__ ((unused));

/************************* End of Prototypes **************************/

/* symbols defined in assembler code */
extern long rwabs();
extern _BPB *getbpb();
extern long mediach();
extern unsigned long old_rwabs, old_getbpb, old_mediach;


/*
 * Mount a partition as GEMDOS drive
 */
int mount( struct tmpmnt *p )
{
	unsigned long mask = 1 << p->drive;
	char letter = p->drive + 'A';
	
	if (p->drive >= MAX_DRIVES) {
		cprintf( "Cannot have more than 16 drives (attempt to mount %c:)\n",
				letter );
		return( 0 );
	}
	if (_drvbits & mask) {
		cprintf( "Attempt to mount drive %c: which is already present\n",
				letter );
		return( 0 );
	}

	install_vectors();
	drv_param[p->drive].mediach = 0;
	drv_param[p->drive].dev     = p->device;
	drv_param[p->drive].start   = p->start_sec;
	drv_param[p->drive].rw      = p->rw;
	my_drives |= mask;
	if (!do_getbpb( p->drive )) {
		cprintf( "New drive %c: contains no valid GEMDOS filesystem\n",
				letter );
		my_drives &= ~mask;
		if (!my_drives)
			uninstall_vectors();
		return( 0 );
	}

	_drvbits |= mask;

	if (Debug)
		cprintf( "Mounted dev %u:%lu on %c: (%s)\n", p->device, p->start_sec,
				p->drive + 'A', p->rw ? "rw" : "ro" );
	
	return( 1 );
}


/*
 * Unmount all our GEMDOS drives
 */
int umount( void )
{
	int i;
	unsigned long mask;
	
	for( i = 0, mask = 1; i < 32; ++i, mask <<= 1 ) {
		if (my_drives & mask) {
			if (!umount_drv( i ))
				cprintf( "Error unmounting drive %c:\n", i+'A' );
			else
				my_drives &= ~mask;
		}
	}
	return( my_drives == 0 );
}


/*
 * Unmount one of our drives
 */
int umount_drv( int drv )
{
	static char *fname = "X:\\X";
	int fd;
	struct drv_param *dp = &drv_param[drv];

	if (!(my_drives & (1 << drv))) {
		cprintf( "Drive %c: not mounted\n", drv+'A' );
		return( 0 );
	}
	
	/* patch drive letter into path name */
	fname[0] = drv + 'A';
	/* set mediach flag, this forces the low-level driver to signal a medium
	 * change and return NULL on next Getbpb() */
	dp->mediach = 1;
	/* Open a file on the drive to get GEMDOS to call Mediach() */
	if ((fd = Fopen( fname, 0 )) >= 0)
		Fclose( fd );

	if (Debug)
		cprintf( "Unmounted drive %c:, mediach=%u\n", drv+'A',dp->mediach );

	/* if mediach flag is still set, some error happened */
	return( dp->mediach == 0 );
}


static void install_vectors( void )
{
	if (VectorsInstalled)
		return;
	old_rwabs = hdv_rw;
	old_getbpb = hdv_getbpb;
	old_mediach = hdv_mediach;
	hdv_rw = (unsigned long)rwabs;
	hdv_getbpb = (unsigned long)getbpb;
	hdv_mediach = (unsigned long)mediach;
	VectorsInstalled = 1;
}


static void uninstall_vectors( void )
{
	if (!VectorsInstalled)
		return;
	remove_vector( &hdv_rw, "hdv_rwabs" ); 
	remove_vector( &hdv_getbpb, "hdv_getbpb" ); 
	remove_vector( &hdv_mediach, "hdv_mediach" ); 
	VectorsInstalled = 0;
}


static int remove_vector( unsigned long *vec, const char *vecname )
{
	XBRA *x;

	for( ; vec; vec = &x->oldvec ) {
		x = (XBRA *)(*vec - 8);
		if (x->magic != *(unsigned long *)"XBRA") {
			cprintf( "Can't remove me from %s vector chain (XBRA violation)\n",
					 vecname );
			return( 0 );
		}
		if (x->id == *(unsigned long *)"LILO") {
			*vec = x->oldvec;
			return( 1 );
		}
	}
	cprintf( "Can't find id \"LILO\" in XBRA chain of %s\n", vecname );
	return( 0 );
}

/* the following assembler functions check whether our drives are concerned by
 * a call. If no, they call the original function; if yes, they jump to our
 * implementation. Usually the do_* function converts its arguments itself,
 * except do_getbb(), which is also called from mount().
 */

__asm__ ( "
	.ascii	\"XBRALILO\"
	.globl	_old_rwabs
_old_rwabs:
	.dc.l	0
_rwabs:
	movel	_my_drives,d0
	moveq	#0,d1
	movew	sp@(4),d1
	btst	d1,d0
	jbne	_do_rwabs
	movel	pc@(_old_rwabs),a0
	jmp		a0@

	.ascii	\"XBRALILO\"
	.globl	_old_getbpb
_old_getbpb:
	.dc.l	0
_getbpb:
	movel	_my_drives,d0
	moveq	#0,d1
	movew	sp@(4),d1
	btst	d1,d0
	jbne	1f
	movel	pc@(_old_getbpb),a0
	jmp		a0@
1:	movel	d1,sp@-				| convert arg from 16 to 32 bit
	jbsr	_do_getbpb
	addql	#4,sp
	rts

	.ascii	\"XBRALILO\"
	.globl	_old_mediach
_old_mediach:
	.dc.l	0
_mediach:
	movel	_my_drives,d0
	moveq	#0,d1
	movew	sp@(4),d1
	btst	d1,d0
	jbne	_do_mediach
	movel	pc@(_old_mediach),a0
	jmp		a0@
");


static long do_rwabs( int first_arg )
{
	/* get 16 bit arguments */
	unsigned short *argp = (unsigned short *)&first_arg;
	unsigned int rwflag = *argp++;
	char    *buffer = *((char **)argp)++;
	unsigned int cnt = *argp++;
	unsigned long sector = *argp++;
	unsigned int drv = *argp++;
	struct drv_param *dp = &drv_param[drv];
	long err;
	
	if (dp->mediach && !(rwflag & 2))
		return( -E_CHNG );

	sector += dp->start;
	if (rwflag & 1) {
		if (!dp->rw)
			err = -EROFS;
		else
			err = WriteSectors( buffer, dp->dev, sector, cnt*dp->secsize );
	}
	else {
		err = WriteSectors( buffer, dp->dev, sector, cnt*dp->secsize );
	}
	if (err && Debug)
		cprintf( "rwabs DMA%s(sec=%lu,dev=%u): error %s \n",
				 (rwflag & 1) ? "write" : "read", sector, dp->dev,
				 tos_perror(err) );
	
	return( err );
}


/* do_getbpb is called with 32bit drv, because it's also called from C code */
static _BPB *do_getbpb( unsigned int drv )
{
	struct drv_param *dp = &drv_param[drv];
	struct boot_sector *boot;
	_BPB *b = &global_bpb;
	int secsize, clusize;
	long err;
	
	if (dp->mediach) {
		dp->mediach = 0;
		return( NULL );
	}

	if ((err = ReadSectors( _dskbuf, dp->dev, dp->start, 1 ))) {
		if (Debug)
			cprintf( "getbpb DMAread(sec=%lu,dev=%u): error %s \n",
					 dp->start, dp->dev, tos_perror(err) );
		return( NULL );
	}
	boot = (struct boot_sector *)_dskbuf;

	secsize = (short)BOOTSEC_FIELD2(boot,sector_size);
	if (secsize < 512 || (secsize & 0xff))
		/* quick check for invalid sector size */
		return( NULL );
	clusize = (signed char)BOOTSEC_FIELD1(boot,cluster_size);
	if (clusize <= 0)
		return( NULL );
	if (BOOTSEC_FIELD1(boot,nfats) != 2)
		return( NULL );

	dp->secsize = secsize;
	b->recsiz = secsize;
	b->clsiz  = clusize;
	b->clsizb = secsize * clusize;
	b->rdlen  = BOOTSEC_FIELD2(boot,dir_entries) * 32 / secsize;
	b->fsiz   = BOOTSEC_FIELD2(boot,fat_length);
	b->fatrec = BOOTSEC_FIELD2(boot,rsvd_sect) + b->fsiz;
	b->datrec = b->fatrec + b->fsiz + b->rdlen;
	b->numcl  = (BOOTSEC_FIELD2(boot,sectors) - b->datrec) / clusize;
	b->bflags = 1; /* always 16bit FAT, since we do only disk filesystems */

	return( b );
}


static long do_mediach( unsigned int _drv )
{
	unsigned int drv = *(unsigned short *)&_drv;
	
	if (drv_param[drv].mediach) {
		drv_param[drv].mediach++;
		return( 2 );
	}
	return( 0 );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
