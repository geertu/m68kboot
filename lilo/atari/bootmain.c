/* bootmain.c -- Main program of Atari LILO booter
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: bootmain.c,v 1.1 1997-08-12 15:27:07 rnhodek Exp $
 * 
 * $Log: bootmain.c,v $
 * Revision 1.1  1997-08-12 15:27:07  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <osbind.h>

#include "bootstrap.h"
#include "linuxboot.h"
#include "config.h"
#include "loader.h"
#include "menu.h"
#include "monitor.h"
#include "parsetags.h"
#include "tmpmnt.h"
#include "sysvars.h"
#include "minmax.h"

const struct FileVectorData MapVectorData = {
    { LILO_ID, LILO_MAPVECTORID }, MAXVECTORSIZE
};

unsigned ExitAction = 0;

unsigned Debug = 0;
unsigned SerialPort = 0;
unsigned AutoBoot = 0;
const char *Prompt = NULL;
unsigned NoGUI = 0;
const struct BootRecord *dflt_os = NULL;
int CurrentFloppy = 0;

u_char *MapData = NULL;
u_long MapSize = 0, MapOffset = 0;
struct BootOptions *BootOptions = NULL;
struct BootRecord *BootRecords = NULL;
struct FileDef *Files = NULL;
struct TagTmpMnt *MountPointList = NULL;
struct tmpmnt *MountPoints;

/* table of some TOS error messages */
struct {
	long num;
	const char *str;
} toserr_table[] = {
	{  -1, "General BIOS error" },
	{  -2, "Drive not ready" },
	{  -4, "CRC error" },
	{  -6, "Seek error" },
	{  -7, "Unknown media" },
	{  -8, "Sector not found" },
	{ -10, "Write fault" },
	{ -11, "Read fault" },
	{ -13, "Write protect" },
	{ -15, "Unknown device" },
	{ -16, "Bad sectors" },
	{ -32, "Invalid function number" },
	{ -33, "File not found" },
	{ -34, "Path not found" },
	{ -36, "Access denied" },
	{ -39, "Insufficient memory" },
	{ -40, "Invalid memory block address" },
	{ -46, "Invalid drive spec" },
	{ -64, "Range error" },
	{ -65, "Internal error" },
	{ -66, "Exec format error" },
	{ -67, "Memory block growth failure" }
};


/***************************** Prototypes *****************************/

static void ReadMapData( void);
static unsigned parse_serial_params( const char *param );
static void patch_cookies( void );
static void set_cookie( const char *name, u_long value );
static void cache_ctrl( int on_flag );
static char *firstword( char **str );

/************************* End of Prototypes **************************/



int main( int argc, char *argv[] )
{
	int i;
	const struct BootRecord *boot_os;
	char *cmdline = NULL, *label;
	
	/* final 'd' of LILO...booted */
	Cconout( 'd' );

	/* enable caches while in the loader */
	cache_ctrl( 1 );
	
	/* read configuration data */
	ReadMapData();

	/* If the user directed us to change cookie values, do so now */
	patch_cookies();

	/* acustic signal if not disabled */
	if (!(BootOptions->NoBing && *BootOptions->NoBing))
		Cconout( 7 );

	/* If 'auto' option is set and no modifier key is pressed, go on booting
	 * the default OS */
	boot_os = dflt_os = FindBootRecord(NULL);
	if (is_available( boot_os )) {
		if (BootOptions->Auto && *BootOptions->Auto && !dflt_os->Password) {
			unsigned long timeout = _hz_200;
			if (BootOptions->Delay)
				timeout += *BootOptions->Delay * HZ;
			do {
				if (Kbshift( -1 ) & 0xff) {
					/* any modifier or mouse button pressed */
					AutoBoot = 1;
					goto boot_default;
				}
			} while( _hz_200 < timeout );
		}
	}
	else
		printf( "Can't use default OS %s, because record is incomplete\n",
				dflt_os->Label );
	
	/* do global temp. mounts and programs */
	for( i = 0; i < MAX_TMPMNT; ++i ) {
		if (BootOptions->TmpMnt[i])
			mount( &MountPoints[*BootOptions->TmpMnt[i]] );
	}
	for( i = 0; i < MAX_EXECPROG; ++i ) {
		if (BootOptions->ExecProg[i])
			exec_tos_program( BootOptions->ExecProg[i] );
	}
	umount();

	/* now initialize the VDI, after we executed the TOS programs (which could
	 * have been gfx board drivers) */
	graf_init( BootOptions->VideoRes );
	
	/* initialize serial port */
	if (BootOptions->Serial) {
		int oldserial;
		u_long serialbaud, serialflow;
		const char *serialparam;
		
		/* allow only AUX: and specific serial devices (numbers >= 6) for
		 * 'serial'; if *BootOptions->Serial is something different, use AUX:
		 * (current, default serial, probably Modem1) */
		SerialPort = *BootOptions->Serial;
		if (SerialPort != 1 && SerialPort < 6)
			SerialPort = 1;
		/* serial parameter, use defaults if not set */
		serialparam = BootOptions->SerialParam ?
					  BootOptions->SerialParam : "8N1";
		serialbaud = BootOptions->SerialBaud ? *BootOptions->SerialBaud : 0;
		serialflow = BootOptions->SerialFlow ? *BootOptions->SerialFlow : 0;
		
		/* make it the current serial port for configuration */
		oldserial = Bconmap( SerialPort );
		Rsconf( serialbaud, serialflow, parse_serial_params( serialparam ),
				-1, -1, -1 ) ;
		Bconmap( oldserial );
	}

	for(;;) {
		for(;;) {
			if (cmdline)
				free( cmdline );
			AutoBoot = 0; /* set again on timeout */
			cmdline = strdup( boot_menu( dflt_os->Label ) );
			if (strcmp( cmdline, "su" ) == 0) {
				if (BootOptions->MasterPassword &&
					strcmp( get_password(),BootOptions->MasterPassword ) !=0) {
					menu_error( "Bad password!" );
					continue;
				}
				boot_monitor();
				continue;
			}
			label = firstword( &cmdline );
			if ((boot_os = FindBootRecord( label ))) {
				if (boot_os->Password &&
					strcmp( get_password(), boot_os->Password ) != 0) {
					menu_error( "Bad password!" );
					free( cmdline );
					continue;
				}
				if (!is_available( boot_os )) {
					printf( "Not all necessary data defined for this OS! "
							"(Error in config file)\n" );
					continue;
				}
				break;
			}
			menu_error( "Unknown OS to boot!" );
		}

	  boot_default:
		/* a last check... */
		if (!is_available( boot_os )) {
			printf( "Boot record for %s incomplete -- this shouldn't happen\n",
					boot_os->Label );
		}
		else {
			switch( *boot_os->OSType ) {
			  case BOS_TOS:
				boot_tos( boot_os );
				break;
			  case BOS_LINUX:
				boot_linux( boot_os, cmdline );
				break;
			  case BOS_BOOTB:
				boot_bootsector( boot_os );
				break;
			  default:
				printf( "Undefined OS type %lu\n", *boot_os->OSType );
			}
		}
	}
	return( 1 );
}

/*
 * Checks whether all necessary data defined for a boot record
 */
int is_available( const struct BootRecord *rec )
{
    u_long type = rec->OSType ? *rec->OSType : BOS_LINUX;

	switch( type ) {

	  case BOS_LINUX:
		if (!rec->Kernel || !FindVector( rec->Kernel ) ||
			(rec->Ramdisk && !FindVector( rec->Ramdisk)))
			return( 0 );
		break;

	  case BOS_TOS:
		if (!rec->TOSDriver)
			return( 0 );
		/* fall through */
	  case BOS_BOOTB:
		if (!rec->BootDev || !rec->BootSec)
			return( 0 );
		break;
		
	}
	return( 1 );
}


void boot_tos( const struct BootRecord *rec )
{
	struct tmpmnt driver_mnt;
	long basepage, err;

	/* mount the driver where the driver resides */
	driver_mnt.device = *rec->BootDev;
	driver_mnt.start_sec = *rec->BootSec;
	driver_mnt.drive = 2; /* always use C: */
	driver_mnt.rw = 0; /* ro mode ok */
	if (!mount( &driver_mnt )) {
		cprintf( "Can't mount drive where TOS driver resides (dev%u:%lu)\n",
				driver_mnt.device, driver_mnt.start_sec );
		return;
	}

	/* load the driver into memory */
	basepage = Pexec( 3, rec->TOSDriver, "\000", NULL );
	/* unmount drive again */
	umount();
	if (basepage < 0) {
		cprintf( "Can't load TOS hd driver \"%s\": %s\n",
				rec->TOSDriver, tos_perror(basepage) );
		return;
	}
	
	/* set _bootdev variable, defines the drive from where AUTO folder progs
	 * and accessories are loaded */
	_bootdev = rec->BootDrv ? *rec->BootDrv : 2; /* use C: as default */

	/* close VDI workstation */
	graf_deinit();
	/* turn off caches before starting the driver, it may assume caches are
	 * off... */
	cache_ctrl( 0 );

	/* now run the driver */
	if ((err = Pexec( (Sversion() < 0x1500) ? 4 : 6, NULL, basepage, NULL))
		< 0) {
		cprintf( "Error on executing TOS hd driver \"%s\": %s\n",
				rec->TOSDriver, tos_perror(err) );
		return;
	}

	/* we can now safely jump back to ROM code, since the hd driver should
	 * have changed hdv_rw, which terminates the boot-try loop */
	ExitAction = 1;
	exit( 0 );
}


void boot_linux( const struct BootRecord *rec, const char *cmdline )
{
	int i, override = 0;
	
	/* do temp. mounts and run programs defined for the boot record */
	for( i = 0; i < MAX_TMPMNT; ++i ) {
		if (rec->TmpMnt[i])
			mount( &MountPoints[*rec->TmpMnt[i]] );
	}
	for( i = 0; i < MAX_EXECPROG; ++i ) {
		if (rec->ExecProg[i])
			exec_tos_program( rec->ExecProg[i] );
	}
	umount();

	debugflag = Debug;
	if (rec->Kernel)
		kernel_name = (char *)rec->Kernel;
	if (rec->Ramdisk)
		ramdisk_name = (char *)rec->Ramdisk;
	if (rec->IgnoreTTRam)
		ignore_ttram = *rec->IgnoreTTRam;
	if (rec->LoadToSTRam)
		load_to_stram = *rec->LoadToSTRam;
	if (rec->ForceSTRam)
		force_st_size = *rec->ForceSTRam;
	if (rec->ForceTTRam)
		force_tt_size = *rec->ForceTTRam;
	if (rec->ExtraMemStart)
		extramem_start = *rec->ExtraMemStart;
	if (rec->ExtraMemSize)
		extramem_size = *rec->ExtraMemSize;

	if (strncmp( cmdline, "override", 8 ) == 0 &&
		(isspace(cmdline[8]) || !cmdline[8])) {
		override = 1;
		cmdline += 8;
		cmdline += strspn( cmdline, " " );
	}
	strncpy( command_line, cmdline, CL_SIZE );
	command_line[CL_SIZE] = 0;
	if (!override && rec->Args) {
		strncat( command_line, rec->Args, CL_SIZE - strlen(command_line) );
		command_line[CL_SIZE] = 0;
	}

	linux_boot();
}


void boot_bootsector( const struct BootRecord *rec )
{
	long err;
    signed short *p, *pend, sum;

	/* close VDI workstation */
	graf_deinit();
	/* turn off caches before loading code */
	cache_ctrl( 0 );

	/* load the boot sector into _dskbuf */
	if ((err = ReadSectors( _dskbuf, *rec->BootDev, *rec->BootSec, 1 ))) {
		printf( "Error reading boot sector dev%lu:%lu: %s\n",
				*rec->BootDev, *rec->BootSec, tos_perror(err) );
		return;
	}

	/* check the checksum */
	p = (signed short *)_dskbuf;
	pend = p + 512/sizeof(short);
    for( sum = 0; p < pend; ++p )
		sum += *p;
	if (sum != 0x1234) {
		printf( "Boot sector on dev%lu:%lu isn't bootable!\n",
				*rec->BootDev, *rec->BootSec );
		return;
	}

	/* ok, jump to it */
	ExitAction = 2;
	exit( 0 );
}

/*
 *	Read the Map Data
 */
static void ReadMapData(void)
{
	struct TagTmpMnt *tmnt;
	struct tmpmnt *mnt;
	unsigned i;
	
	/* parse tags */
    ParseTags();

	Prompt = BootOptions->Prompt ? BootOptions->Prompt : "LILO boot: ";
	NoGUI = BootOptions->NoGUI && *BootOptions->NoGUI;
	
	/* make an array of the tmpmnt list */
	for( i = 0, tmnt = MountPointList; tmnt; tmnt = tmnt->next )
		++i;
	if (!(MountPoints = malloc( i*sizeof(struct tmpmnt) )))
		Alert(AN_LILO|AG_NoMemory);
	for( i = 0, tmnt = MountPointList; tmnt; tmnt = tmnt->next, ++i ) {
		mnt = &MountPoints[i];
		if (!tmnt->device || !tmnt->drive ||
			!tmnt->start_sec || !tmnt->rw) {
			fprintf( stderr, "Incomplete mount entry #%d\n", i );
			continue;
		}
		mnt->device    = *tmnt->device;
		mnt->drive     = *tmnt->drive;
		mnt->start_sec = *tmnt->start_sec;
		mnt->rw        = *tmnt->rw;
	}
}


/*
 * Execute a TOS program (from a mounted drive)
 */
int exec_tos_program( const char *prog )
{
	unsigned cmdlen, arglen;
	const char *p;
	char *cmd, *args;
	long err;
	
	/* extract program name */
	cmdlen = strspn( prog, " \t" );
	if (!(cmd = malloc( cmdlen+1 )))
		Alert(AN_LILO|AG_NoMemory);
	strncpy( cmd, prog, cmdlen );
	cmd[cmdlen] = 0;

	/* extract arguments */
	p = prog + cmdlen;
	p += strspn( p, " \t" );
	arglen = min( strlen(p), 124 );
	if (!(args = malloc( arglen+2 )))
		Alert(AN_LILO|AG_NoMemory);
	strncpy( args+1, p, arglen );
	args[0] = arglen;
	args[arglen+1] = 0;

	if (Debug)
		printf( "Executing %s %s\n", cmd, args );

	err = Pexec( 0, cmd, args, NULL );
	if (err < 0) {
		/* if negative as 32bit number, it was a GEMDOS error */
		printf( "Error on executing %s: %s\n", cmd, tos_perror(err) );
		tos_perror( err );
		return( -1 );
	}
	else
		return( err );
}


/*
 * Return description for a TOS error number
 */
const char *tos_perror( long err )
{
	int i;
	static char buf[10];
	
	for( i = 0; i < arraysize(toserr_table); ++i ) {
		if (toserr_table[i].num == err)
			return( toserr_table[i].str );
	}
	sprintf( buf, "%ld", err );
	return( buf );
}


/*
 * Translate a serial parameter string (e.g. "8N1") to an ucr value for
 * Rsconf()
 */
static unsigned parse_serial_params( const char *param )
{
	unsigned bits = 0, parity = 0, stopbits = 1;

	if (*param >= '5' && *param <= '8')
		bits = 3 - (*param - '5');
	else
		printf( "Bad number of data bits (%c) in serial parameters\n",
				*param );
	if (*param)
		++param;

	switch( tolower(*param) ) {
	  case 'n':
		parity = 0;
		break;
	  case 'o':
		parity = 2;
		break;
	  case 'e':
		parity = 3;
		break;
	  default:
		printf( "Bad parity specification (%c) in serial parameters\n",
				*param );
	}
	if (*param)
		++param;

	if (*param >= '1' && *param <= '2') {
		stopbits = (*param - '1')*2+1;
		if (param[1] == '.' && param[2] == '5') {
			++stopbits;
			param += 2;
		}
	}
	else
		printf( "Bad number of stop bits (%c) in serial parameters\n",
				*param );
	if (*param)
		++param;

	if (*param)
		printf( "Junk at end of serial parameter string (%s)\n", param );

	return( (bits << 5) | (stopbits << 3) | (parity << 1) );
}


/*
 * Change _MCH, _CPU, and _FPU cookies according to configuration
 */
static void patch_cookies( void )
{
	if (!*_p_cookies) {
		printf( "No cookie jar -- can't change cookie values.\n" );
		return;
	}
	if (BootOptions->MCH_cookie)
		set_cookie( "_MCH", *BootOptions->MCH_cookie );
	if (BootOptions->CPU_cookie)
		set_cookie( "_CPU", *BootOptions->CPU_cookie );
	if (BootOptions->FPU_cookie)
		set_cookie( "_FPU", *BootOptions->FPU_cookie );
}


/*
 * Change value of a single cookie
 */
static void set_cookie( const char *name, u_long value )
{
	u_long *jar = *_p_cookies, jarlen;

	while( *jar ) {
		if (*jar == *(u_long *)name) {
			jar[1] = value;
			return;
		}
		jar += 2;
	}

	jarlen = jar[1];
	if (jar - *_p_cookies < jarlen-1) {
		/* add new cookie at end, shift end marker by one slot */
		jar[0] = *(u_long *)name;
		jar[1] = value;
		jar[2] = 0;
		jar[3] = jarlen;
	}
	else
		printf( "%s cookie not found and jar full. Can't add it.\n", name );
}


/*
 * Turn on or off caches
 */
static void cache_ctrl( int on_flag )
{
	static unsigned long old_cacr, old_dtt0, old_itt0, old_dtt1, old_itt1;
	
	if (on_flag) {
		__asm__ __volatile__ (
			/* redirect illegal insn vector to catch wrong CPU case */
			"	movel	sp,a1\n"
			"	movel	0x10,a0\n"
			"	movel	#1f,0x10\n"
			/* '040 and '060 case: set up transp. transl. for whole mem, set
			 * cacr bits */
			"	.chip	68040\n"
			"	movec	dtt0,%0\n"
			"	movec	itt0,%1\n"
			"	movec	dtt1,%2\n"
			"	movec	itt1,%3\n"
			"	movel	#0x00ffc000,d0\n" /* everywhere, user&super, cache/wt*/
			"	movec	d0,dtt0\n"
			"	movel	#0x007fc000,d0\n" /* lower 2GB, user&super, cache/wt */
			"	movec	d0,itt0\n"
			"	moveq	#0,d0\n"
			"	movec	d0,dtt1\n"
			"	movec	d0,itt1\n"
			"	movec	cacr,%4\n"
			"	movel	#0xa0008000,d0\n" /* enable I+D cache, enable '060
										   * store buffer, on '060, don't
										   * enable branch target cache */
			"	movec	d0,cacr\n"
			"	bra		2f\n"
			"1:	\n"
			/* '030 case: simply set some cacr bits: both caches on with burst
			 * mode, clear caches (write allocation is left out, because some
			 * extensions have problems with it...) */
			"	.chip	68030\n"
			"	movel	#0x1919,d0\n"
			"	movec	d0,cacr\n"
			/* clean up */
			"	.chip	68k\n"
			"2:	movel	a0,0x10\n"
			"	movel	a1,sp"
			: "=d" (old_dtt0), "=d" (old_itt0),
			  "=d" (old_dtt1), "=d" (old_itt1),
			  "=d" (old_cacr)
			: /* no inputs */
			: "d0", "a0", "a1" );
	}
	else {
		__asm__ __volatile__ (
			/* redirect illegal insn vector to catch wrong CPU case */
			"	movel	sp,a1\n"
			"	movel	0x10,a0\n"
			"	movel	#1f,0x10\n"
			/* '040 and '060: invalidate caches, restore old cacr and transp.
			 * transl. registers */
			"	.chip	68040\n"
			"	cpusha	%%bc\n"
			"	movec	%0,dtt0\n"
			"	movec	%1,itt0\n"
			"	movec	%2,dtt1\n"
			"	movec	%3,itt1\n"
			"	movec	%4,cacr\n"
			"	bra		2f\n"
			/* '030: clear caches and turn them off */
			"	.chip	68030\n"
			"1:	moveq	#0,d0\n"
			"	movec	d0,cacr\n" /* caches off */
			"	movel	#0x0808,d0\n"
			"	movec	d0,cacr\n" /* clear caches */
			/* clean up */
			"	.chip	68k\n"
			"2:	movel	a0,0x10\n"
			"	movel	a1,sp"
			: /* no outputs */
			: "d" (old_dtt0), "d" (old_itt0),
			  "d" (old_dtt1), "d" (old_itt1),
			  "d" (old_cacr)
			: "d0", "a0", "a1" );
	}
}


/*
 * Extract first word (whitespace-separated) from *str and return pointer to
 * it; *str is modified to point to rest of line
 */
static char *firstword( char **str )
{
	char *p = *str, *q;
	
	while( *p && isspace(*p) )
		++p;
	q = p;
	while( *p && !isspace(*p) )
		++p;
	*p++ = 0;
	while( *p && isspace(*p) )
		++p;
	*str = p;
	
	return( q );
}



void MachInitDebug( void )
{
	/* no-op on Atari */
}

char *AlertText[] = {
	"No memory",
	"Bad map file"
};

/* Alert() is used in common code */
void Alert( enum AlertCodes code )
{
	fprintf( stderr, "Error: %s\n", AlertText[code] );
}


/*
 * Read or write some sectors
 * (ReadSectors is also a callback for file_mod.c)
 *
 * Device numbering:
 *   device >= 0 is a hard disk, standard TOS numbering (as with DMAread/write)
 *   device == -1 is the 'current' floppy, i.e. the drive the bootsector was
 *                loaded from
 *   device == -2 is floppy drive A:
 *   device == -3 is floppy drive B:
 */

/* Callback for file_mod.c: Read some sectors */
long ReadSectors( char *buf, unsigned _device, unsigned sector, unsigned cnt )
{
	int device = _device;
	
	if (device < 0) {
		device = -device - 2;
		if (device < 0) device = CurrentFloppy;
		return( Rwabs( 2, buf, cnt, sector, device ) );
	}
	else
		return( DMAread( sector, cnt, buf, device ) );
}

long WriteSectors( char *buf, int device, unsigned sector, unsigned cnt )
{
	if (device < 0) {
		device = -device - 2;
		if (device < 0) device = CurrentFloppy;
		return( Rwabs( 3, buf, cnt, sector, device ) );
	}
	else
		return( DMAwrite( sector, cnt, buf, device ) );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
