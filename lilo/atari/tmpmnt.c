/*
 * tmpmnt.c -- Temporary mounts for Atari Linux
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: tmpmnt.c,v 1.7 1998-03-16 10:48:29 schwab Exp $
 * 
 * $Log: tmpmnt.c,v $
 * Revision 1.7  1998-03-16 10:48:29  schwab
 * In umount_drv(), put path template into data section.
 *
 * Revision 1.6  1998/02/27 10:22:00  rnhodek
 * Removed #ifdef DEBUG_RW_SECTORS.
 *
 * Revision 1.5  1998/02/26 10:34:08  rnhodek
 * my_drive and vector maintainance on umounting should be done by umount_drv(),
 * not umount().
 * umount_drv() should also clear bit in _drvbits.
 * New function list_mounts().
 * unstall_vectors(): sizeof(XBRA) is 12, not 8.
 * _rwabs: drive parameter is at offset 14, not 4.
 * do_* functions: add debugging printfs.
 * do_rwabs: Recognize lsector argument (if sector==-1); sector must also
 * be multiplied with logical sector size.
 * do_getbpb: Should not return error if mediach set, but reset the flag;
 * dp->secsize should be multiplier for HARD_SECTOR_SIZE (needs >> 9);
 * set up b->bflags correctly (we can mount floppies).
 *
 * Revision 1.4  1998/02/25 10:38:35  rnhodek
 * umount_drv() should also clear bit in _drvbits.
 *
 * Revision 1.3  1998/02/24 11:22:29  rnhodek
 * Fix indentation.
 *
 * Revision 1.2  1997/09/19 09:07:00  geert
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
		}
	}
	return( my_drives == 0 );
}


/*
 * Unmount one of our drives
 */
int umount_drv( int drv )
{
	static char fname[] = "X:\\X";
	int fd;
	struct drv_param *dp = &drv_param[drv];
	unsigned long mask = ~(1 << drv);

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

	/* if mediach flag is still set, some error happened */
	if (dp->mediach != 0)
		return( 0 );
	
	_drvbits &= mask;
	my_drives &= mask;
	if (Debug)
		cprintf( "Unmounted drive %c:, mediach=%u\n", drv+'A',dp->mediach );
	if (!my_drives)
		uninstall_vectors();
	return( 1 );
}


void list_mounts( void )
{
	int i;
	struct drv_param *dp;
	
	for( i = 0; i < MAX_DRIVES; ++i ) {
		if (!(my_drives & (1 << i)))
			continue;
		dp = &drv_param[i];
		if (dp->dev >= 16)
			cprintf( "  IDE%d", dp->dev - 16 );
		else if (dp->dev >= 8)
			cprintf( "  SCSI%d", dp->dev - 8 );
		else if (dp->dev >= 0)
			cprintf( "  ACSI%d", dp->dev );
		else
			cprintf( "  FLOP%s", dp->dev == -2 ? "A": dp->dev == -3 ? "B":"" );
		cprintf( ":%lu on %c: (%s,logsecsize=%lu)\n",
				 dp->start, i+'A', dp->rw ? "rw" : "ro", dp->secsize << 9 );
	}
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
		x = (XBRA *)(*vec) - 1;
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
	movew	sp@(14),d1
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

	if (sector == 0xffff)
		sector = *(unsigned long *)argp;
	
	bios_printf( "rwabs(rw=%u,cnt=%u,sect=%lu,dev=%u)\n",rwflag,cnt,sector,drv);
	if (dp->mediach && !(rwflag & 2)) {
		bios_printf( "rwabs: mediach=%d, returning -E_CHNG\n", dp->mediach );
		return( -E_CHNG );
	}

	sector *= dp->secsize;
	sector += dp->start;
	cnt *= dp->secsize;
	if (rwflag & 1) {
		if (!dp->rw)
			err = -EROFS;
		else
			err = WriteSectors( buffer, dp->dev, sector, cnt );
	}
	else {
		err = ReadSectors( buffer, dp->dev, sector, cnt );
	}
	if (err)
		bios_printf( "rwabs DMA%s(sec=%lu,dev=%u): error %s \n",
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
	
	bios_printf( "getbpb(drv=%u)\n",drv);
	dp->mediach = 0;

	if ((err = ReadSectors( _dskbuf, dp->dev, dp->start, 1 ))) {
		bios_printf( "getbpb DMAread(sec=%lu,dev=%u): error %s \n",
					 dp->start, dp->dev, tos_perror(err) );
		return( NULL );
	}
	boot = (struct boot_sector *)_dskbuf;

	secsize = (short)BOOTSEC_FIELD2(boot,sector_size);
	if (secsize < 512 || (secsize & 0x1ff)) {
		/* quick check for invalid sector size */
		bios_printf( "getbpb: invalid sector size %d\n", secsize);
		return( NULL );
	}
	clusize = (signed char)BOOTSEC_FIELD1(boot,cluster_size);
	if (clusize <= 0) {
		bios_printf( "getbpb: invalid cluster size %d\n", clusize);
		return( NULL );
	}
	if (BOOTSEC_FIELD1(boot,nfats) != 2) {
		bios_printf( "getbpb: bat number of fats: %d\n", BOOTSEC_FIELD1(boot,nfats));
		return( NULL );
	}

	dp->secsize = secsize >> 9;
	b->recsiz = secsize;
	b->clsiz  = clusize;
	b->clsizb = secsize * clusize;
	b->rdlen  = BOOTSEC_FIELD2(boot,dir_entries) * 32 / secsize;
	b->fsiz   = BOOTSEC_FIELD2(boot,fat_length);
	b->fatrec = BOOTSEC_FIELD2(boot,rsvd_sect) + b->fsiz;
	b->datrec = b->fatrec + b->fsiz + b->rdlen;
	b->numcl  = (BOOTSEC_FIELD2(boot,sectors) - b->datrec) / clusize;
	b->bflags = (dp->dev < 0) ? 0 /* 12 bit FAT */: 1 /* 16 bit FAT */;

	bios_printf( "getbpb: bootsector ok\n");
	return( b );
}


static long do_mediach( unsigned int _drv )
{
	unsigned int drv = *(unsigned short *)&_drv;
	
	bios_printf( "mediach(drv=%u) mediach=%d\n",drv,drv_param[drv].mediach);
	if (drv_param[drv].mediach) {
		drv_param[drv].mediach++;
		return( 2 );
	}
	return( 0 );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
