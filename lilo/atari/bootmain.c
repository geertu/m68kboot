/* bootmain.c -- Main program of Atari LILO booter
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: bootmain.c,v 1.17 1998-04-07 10:01:43 rnhodek Exp $
 * 
 * $Log: bootmain.c,v $
 * Revision 1.17  1998-04-07 10:01:43  rnhodek
 * Add some more TOS errors.
 *
 * Revision 1.16  1998/03/16 11:11:30  rnhodek
 * Remove Dsetdrv() from boot_tos(), it has no effect anyway (due to Pterm).
 * Instead, check if _bootdev exists and pick another one if not.
 *
 * Revision 1.15  1998/03/16 10:45:18  schwab
 * Do auto boot wait loop as do..while.
 * Rename putenv to Putenv to avoid name clash with lib.
 *
 * Revision 1.14  1998/03/10 10:25:18  rnhodek
 * New option "message": print before showing the prompt.
 * When waiting for auto boot, also stop on chars from the serial port.
 * Add "help" to list of valid commands.
 * New boot record option "restricted": modify test when to prompt for password.
 * ListRecords(): change text printed for passwd-protected/restricted records.
 *
 * Revision 1.13  1998/03/05 12:29:48  rnhodek
 * Replace another printf by cprintf.
 *
 * Revision 1.12  1998/03/05 11:09:53  rnhodek
 * Also show boot records if "help" is entered.
 *
 * Revision 1.11  1998/03/05 10:25:34  rnhodek
 * In NO_GUI mode, if user types '?' at the boot prompt, print out a
 * list of available boot records.
 * New function ListRecords() for this, copied from monitor.c.
 *
 * Revision 1.10  1998/03/04 09:14:47  rnhodek
 * New option 'use_cache' to exec_tos_program().
 * New config var array ProgCache[].
 * Remove strace_{on,off} calls.
 * New function cache_invalidate().
 * Call cache_invalidate() after reading sectors in ReadSectors().
 *
 * Revision 1.9  1998/03/02 13:06:45  rnhodek
 * Introduce #ifdefs for NO_GUI and NO_MONITOR compile time options.
 * NoGUI renamed to DontUseGUI.
 * Initialize serial port earlier (before autoboot).
 * Show a banner (Lilo version and authors).
 * Do bing also on serial terminal.
 * Show "countdown" during autoboot.
 * Call setup_environ() after mounting TOS drives. Otherwise the selected
 * boot-drive may not be available yet.
 * New function goto_last_line() replaces going to last line in
 * graf_deinit(), which was called at wrong times for that job.
 * In boot_tos, initialize _bootdev later (after HD driver installed);
 * check that selected drive is valid and also select it as current drive.
 * boot_linux(): Separate parts of command line with a space.
 * exec_tos_program(): no need to save old cwd; check that drive is
 * available before setting it.
 *
 * Revision 1.8  1998/02/27 10:19:03  rnhodek
 * Include call to experimental strace_tos (for debugging).
 * Removed #ifdef DEBUG_RW_SECTORS.
 *
 * Revision 1.7  1998/02/26 11:17:08  rnhodek
 * Print CR-NL after 'd' of LILO...booted.
 *
 * Revision 1.6  1998/02/26 10:15:57  rnhodek
 * Implement new config vars Environ and BootDrv with functions
 * setup_environ() and putenv().
 * Print autoboot message also if Debug == 0.
 * strspn in exec_tos_program() should have been strcspn.
 * New option 'workdir' to exec_tos_program(), to implement new config
 * var 'WorkDir'.
 * Init VDI only if NoGUI == 0.
 * Move umount() call after the menu, so that a VDI driver still can
 * access files during menu display.
 * Handle empty command line (means default OS).
 * In boot_tos(), the driver name must be prefixed with "C:\\", otherwise
 * it is searched for on the current drive (which is A:).
 * In cache_ctrl(0), also need to catch Line-F vector, since first '040
 * insn is cpush, which has a 0xfxxx encoding.
 * In first_word(), handle case of empty command line correctly.
 * New function bios_printf() for debugging {Read,Write}Sectors and the
 * hdv_* implementations in tmpmount.c. These are called by GEMDOS and
 * thus can't use GEMDOS functions.
 *
 * Revision 1.5  1998/02/24 11:16:42  rnhodek
 * Fix typo in boot_linux() (printed "TOS" instead of "Linux" :-)
 * Fix [id]tt0 usage in cache_ctrl(): whole instruction space should be
 * cached, but only low 2 GB of data space (for hardware regs).
 * Use cprintf() in Alert(), too.
 *
 * Revision 1.4  1998/02/23 10:16:57  rnhodek
 * Moved definition of CurrentFloppy to crt0.S
 *
 * Revision 1.3  1997/09/19 09:06:55  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.2  1997/08/23 20:47:50  rnhodek
 * Fixed waiting for keypress in autoboot
 * Added some debugging printf
 * Replaced printf by cprintf
 *
 * Revision 1.1  1997/08/12 15:27:07  rnhodek
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
#include "strace_tos.h"
#include "minmax.h"
#include "lilo_common.h" /* for LILO_VERSION */

const struct FileVectorData MapVectorData = {
    { LILO_ID, LILO_MAPVECTORID }, MAXVECTORSIZE
};

unsigned int ExitAction = 0;

int Debug = 0;
unsigned int SerialPort = 0;
unsigned int AutoBoot = 0;
const char *Prompt = NULL;
#ifndef NO_GUI
unsigned int DontUseGUI = 0;
#endif
const struct BootRecord *dflt_os = NULL;
int CacheOn = 0;

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
	{  -3, "Unknown command" },
	{  -4, "CRC error" },
	{  -5, "Bad request" },
	{  -6, "Seek error" },
	{  -7, "Unknown media" },
	{  -8, "Sector not found" },
	{  -9, "Out of paper" },
	{ -10, "Write fault" },
	{ -11, "Read fault" },
	{ -12, "General error" },
	{ -13, "Write protect" },
	{ -15, "Unknown device" },
	{ -16, "Bad sectors" },
	{ -17, "Insert other disk" },
	{ -18, "Insert other disk" },
	{ -19, "Drive not responding" },
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
static unsigned int parse_serial_params( const char *param );
static void patch_cookies( void );
static void setup_environ( void );
static void Putenv( const char *str );
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
	Cconws( "d\r\n" );

	/* enable caches while in the loader */
	cache_ctrl( 1 );
	
	/* read configuration data */
	ReadMapData();

	/* If the user directed us to change cookie values, do so now */
	patch_cookies();

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

	/* print banner */
	cprintf( LILO_VERSION );
	
	/* acustic signal if not disabled */
	if (!(BootOptions->NoBing && *BootOptions->NoBing))
		cprintf( "\a" );

	/* If 'auto' option is set and no modifier key is pressed, go on booting
	 * the default OS */
	boot_os = dflt_os = FindBootRecord(NULL);
	if (Debug)
		cprintf( "Default OS is %s\n", dflt_os->Label );
	if (is_available( boot_os )) {
		if (BootOptions->Auto && *BootOptions->Auto && !dflt_os->Password) {
			unsigned long timebase = _hz_200;
			unsigned long timeout = timebase;
			unsigned long sec, oldsec = 0;
			if (BootOptions->Delay && *BootOptions->Delay) {
				timeout += *BootOptions->Delay * HZ;
				cprintf( "Auto boot in progress.\n"
						 "Waiting %ld seconds for shift key: ",
						 *BootOptions->Delay );
			}
			do {
				sec = (_hz_200 - timebase)/HZ;
				if (sec != oldsec) {
					cprintf( "." );
					oldsec = sec;
				}
				if ((Kbshift( -1 ) & 0xff) || serial_instat()) {
					/* any modifier or mouse button pressed */
					cprintf( "\nAuto boot canceled.\n" );
					if (serial_instat())
						serial_getc();
					goto no_auto_boot;
				}
			} while (_hz_200 < timeout);
			cprintf( "\nDoing auto boot.\n" );
			AutoBoot = 1;
			goto boot_default;
		}
	}
	else
		printf( "Can't use default OS %s, because record is incomplete\n",
				dflt_os->Label );
	
  no_auto_boot:
	/* do global temp. mounts and programs */
	if (Debug)
		cprintf( "Mounting and executing\n" );
	for( i = 0; i < MAX_TMPMNT; ++i ) {
		if (BootOptions->TmpMnt[i])
			mount( &MountPoints[*BootOptions->TmpMnt[i]] );
	}
	/* set up _bootdev and environment */
	setup_environ();
	for( i = 0; i < MAX_EXECPROG; ++i ) {
		if (BootOptions->ExecProg[i])
			exec_tos_program( BootOptions->ExecProg[i],
							  BootOptions->WorkDir[i],
							  BootOptions->ProgCache[i] ?
							  *BootOptions->ProgCache[i] : 1 );
	}

	/* If a message is defined, display it now after processing the auto boot
	 * and initializing a video board, but before opening the VDI workstation
	 * (which clears the screen) */
	if (BootOptions->Message) {
		cprintf( "%s\n", BootOptions->Message );
		/* Wait for a key before clearing the screen */
		if (!DontUseGUI) {
			cprintf( "Press any key to continue\n" );
			waitkey();
			cprintf( "\n" );
		}
	}
	
	/* now initialize the VDI, after we executed the TOS programs (which could
	 * have been gfx board drivers) */
	if (!DontUseGUI)
		graf_init( BootOptions->VideoRes );
	
	for(;;) {
		for(;;) {
			if (cmdline)
				free( cmdline );
			AutoBoot = 0; /* set again on timeout */
			cmdline = strdup( boot_menu( dflt_os->Label ) );
#ifndef NO_MONITOR
			if (strcmp( cmdline, "su" ) == 0) {
				if (BootOptions->MasterPassword &&
					strcmp( get_password(),BootOptions->MasterPassword ) !=0) {
					menu_error( "Bad password!" );
					continue;
				}
				boot_monitor();
				continue;
			}
#endif
			label = firstword( &cmdline );
			if (DontUseGUI &&
				(strcmp( label, "?" ) == 0 || strcmp( label, "help" ) == 0)) {
				cprintf( "\nValid commands: ?, help"
#ifndef NO_MONITOR
						 ", su"
#endif
						 "\n" );
				ListRecords();
				cprintf( "\n" );
				continue;
			}
			if ((boot_os = (*label ? FindBootRecord( label ) : dflt_os))) {
				/* Prompt for a password if there is one and either:
				 *  - Restricted is false
				 *  - Restricted is true and there's a command line
				 */
				if (boot_os->Password &&
					(!(boot_os->Restricted && *boot_os->Restricted) ||
					 *cmdline)) {
					if (strcmp( get_password(), boot_os->Password ) != 0) {
						menu_error( "Bad password!" );
						free( cmdline );
						continue;
					}
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
			/* Unmount mounted TOS drives before booting */
			goto_last_line();
			umount();
			
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
				cprintf( "Undefined OS type %lu\n", *boot_os->OSType );
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
	char driver[20];

	/* mount the driver where the driver resides */
	if (Debug)
		cprintf( "Booting TOS -- mounting dev=%ld sec=%lu on C:\n",
				 *rec->BootDev, *rec->BootSec );
	driver_mnt.device = *rec->BootDev;
	driver_mnt.start_sec = *rec->BootSec;
	driver_mnt.drive = 2; /* always use C: */
	driver_mnt.rw = 0; /* ro mode ok */
	if (!mount( &driver_mnt )) {
		cprintf( "Can't mount drive where TOS driver resides (dev%u:%lu)\n",
				driver_mnt.device, driver_mnt.start_sec );
		return;
	}

	strcpy( driver, "C:\\" );
	strncat( driver, rec->TOSDriver, 12 );
	driver[15] = 0;
	
	/* load the driver into memory */
	basepage = Pexec( 3, driver, "\000", NULL );
	/* unmount drive again */
	umount();
	if (basepage < 0) {
		cprintf( "Can't load TOS hd driver \"%s\": %s\n",
				rec->TOSDriver, tos_perror(basepage) );
		return;
	}
	if (Debug)
		cprintf( "TOS HD driver loaded at 0x%08lx\n", basepage );
	
	/* close VDI workstation */
	graf_deinit();
	/* turn off caches before starting the driver, it may assume caches are
	 * off... */
	cache_ctrl( 0 );

	/* now run the driver */
	if (Debug)
		cprintf( "Executing TOS HD driver...\n" );
	if ((err = Pexec( (Sversion() < 0x1500) ? 4 : 6, NULL, basepage, NULL))
		< 0) {
		cprintf( "Error on executing TOS hd driver \"%s\": %s\n",
				rec->TOSDriver, tos_perror(err) );
		return;
	}
	if (Debug)
		cprintf( "OK, return value %ld\n", err );

	/* set _bootdev variable, defines the drive from where AUTO folder progs
	 * and accessories are loaded */
	_bootdev = rec->BootDrv ? *rec->BootDrv : 2; /* use C: as default */
	if (!(_drvbits & (1 << _bootdev))) {
		cprintf( "Warning: Can't set current drive to %c:, "
				 "drive not available\n", _bootdev + 'A' );
		/* GEMDOS doesn't like it if Dsetdrv is called for a non-existant
		 * drive, so choose some other one. If C: exists, use that, otherwise
		 * A:. */
		_bootdev = (_drvbits & (1 << 2)) ? 2 : 0;
		cprintf( "Using %c: as boot drive.\n", _bootdev + 'A' );
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
	if (Debug)
		cprintf( "Booting Linux -- doing tmp mounts and executes\n" );
	for( i = 0; i < MAX_TMPMNT; ++i ) {
		if (rec->TmpMnt[i])
			mount( &MountPoints[*rec->TmpMnt[i]] );
	}
	for( i = 0; i < MAX_EXECPROG; ++i ) {
		if (rec->ExecProg[i])
			exec_tos_program( rec->ExecProg[i], rec->WorkDir[i],
							  rec->ProgCache[i] ? *(rec->ProgCache[i]) : 1 );
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
		if (command_line[0])
			strcat( command_line, " " );
		strncat( command_line, rec->Args, CL_SIZE - strlen(command_line) );
		command_line[CL_SIZE] = 0;
	}
	if (Debug)
		cprintf( "Command line is \"%s\"\n", command_line );

	linux_boot();
}


void boot_bootsector( const struct BootRecord *rec )
{
	long err;
    signed short *p, *pend, sum;

	if (Debug)
		cprintf( "Booting boot sector %lu from dev %ld\n",
				 *rec->BootSec, *rec->BootDev );
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
	if (Debug)
		cprintf( "Boot sector loaded, checksum ok, now jumping to it\n" );
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
	unsigned int i;
	
	/* parse tags */
    ParseTags();

	Prompt = BootOptions->Prompt ? BootOptions->Prompt : "LILO boot: ";
#ifndef NO_GUI
	DontUseGUI = BootOptions->NoGUI && *BootOptions->NoGUI;
#endif
	
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


void ListRecords( void )
{
	const struct BootRecord *rec;
	
	cprintf( "Boot records:\n" );
	for( rec = BootRecords; rec; rec = rec->Next ) {
		cprintf( "  %-31.31s (", rec->Label );
		if (rec->Alias)
			cprintf( "alias \"%s\", ", rec->Alias );
		cprintf( "type " );
		switch( rec->OSType ? *rec->OSType : BOS_LINUX ) {
		  case BOS_TOS:		cprintf( "TOS" ); break;
		  case BOS_LINUX:	cprintf( "Linux" ); break;
		  case BOS_BOOTB:	cprintf( "bootsector" ); break;
		  default:			cprintf( "unknown" ); break;
		}
		if (rec->Password)
			cprintf( ", password" );
		if (rec->Restricted)
			cprintf( " for cmd line" );
		if (!is_available( rec ))
			cprintf( ", incomplete" );
		if (rec == dflt_os)
			cprintf( ", default" );
		cprintf( ")\n" );
	}
}


/*
 * Execute a TOS program (from a mounted drive)
 */
int exec_tos_program( const char *prog, const char *workdir, int use_cache )
{
	unsigned int cmdlen, arglen;
	const char *p;
	char *cmd, *args;
	long err;

	/* extract program name */
	cmdlen = strcspn( prog, " \t" );
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

	if (!use_cache)
		cache_ctrl( 0 );
	
	if (workdir) {
		int drv = -1;
		const char *path = workdir;

		if (isalpha(path[0]) && path[1] == ':') {
			drv = toupper(path[0]) - 'A';
			path += 2;
		}

		if (drv >= 0 && !(_drvbits & (1 << drv))) {
			cprintf( "Warning: Can't chdir to %s: drive %c: doesn't exist\n",
					 workdir, drv+'A' );
		}
		else {
			if (Debug)
				cprintf( "Changing directory to %s\n", workdir );
			if (drv >= 0)
				Dsetdrv( drv );
			if ((err = Dsetpath( path )))
				cprintf( "Dsetpath(%s): %s\n", path, tos_perror(err) );
		}
	}
	
	if (Debug)
		cprintf( "Executing %s %s\n", cmd, args+1 );

	err = Pexec( 0, cmd, args, NULL );

	if (workdir && (_drvbits & (1 << _bootdev))) {
		/* reset cwd to <_bootdev>:\ */
		Dsetdrv( _bootdev );
		Dsetpath( "\\" );
	}
	
	if (!use_cache)
		cache_ctrl( 1 );
	
	if (err < 0) {
		/* if negative as 32bit number, it was a GEMDOS error */
		cprintf( "Error on executing %s: %s\n", cmd, tos_perror(err) );
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
static unsigned int parse_serial_params( const char *param )
{
	unsigned int bits = 0, parity = 0, stopbits = 1;

	if (*param >= '5' && *param <= '8')
		bits = 3 - (*param - '5');
	else
		cprintf( "Bad number of data bits (%c) in serial parameters\n",
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
		cprintf( "Bad parity specification (%c) in serial parameters\n",
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
		cprintf( "Bad number of stop bits (%c) in serial parameters\n",
				 *param );
	if (*param)
		++param;

	if (*param)
		cprintf( "Junk at end of serial parameter string (%s)\n", param );

	return( (bits << 5) | (stopbits << 3) | (parity << 1) );
}


/*
 * Change _MCH, _CPU, and _FPU cookies according to configuration
 */
static void patch_cookies( void )
{
	if (!*_p_cookies) {
		cprintf( "No cookie jar -- can't change cookie values.\n" );
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
 * Set up _bootdev and environment as directed by the configuration
 */
static void setup_environ( void )
{
	int i;
	
	if (BootOptions->BootDrv) {
		char env_path[4] = "X:\\";
		_bootdev = *BootOptions->BootDrv;
		/* create a BIOS compatible PATH= variable (same strange syntax...) */
		Putenv( "PATH=" );
		env_path[0] = *BootOptions->BootDrv + 'A';
		Putenv( env_path );
		/* change current dir to <_bootdrv>:\ */
		if (_drvbits & (1 << _bootdev)) {
			Dsetdrv( *BootOptions->BootDrv );
			Dsetpath( "\\" );
		}
		else
			cprintf( "Warning: Can't chdir to configured BootDrv %c:, "
					 "not mounted\n", _bootdev + 'A' );
	}

	for( i = 0; i < MAX_ENVIRON; ++i ) {
		if (BootOptions->Environ[i])
			Putenv( BootOptions->Environ[i] );
	}
}


/*
 * Simple version of putenv() (doesn't check for existing vars) that writes to
 * the 1280 bytes available after our basepage. Later we could terminate with
 * Ptermres() and keep this space resident, so we can push our enviroment also
 * to other processes started later by TOS.
 */
static void Putenv( const char *str )
{
	static char *env_end = NULL;
	extern char *_base; /* really (BASEPAGE *) */

	if (!env_end)
		env_end = _base + 256;

	while(  *str )
		*env_end++ = *str++;
	*env_end++ = 0;
	*env_end = 0;
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
			"	movec	d0,itt0\n"
			"	movel	#0x007fc000,d0\n" /* lower 2GB, user&super, cache/wt */
			"	movec	d0,dtt0\n"
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
			/* redirect illegal insn and line-F vectors to catch wrong CPU
			 * case */
			"	movel	sp,a1\n"
			"	movel	0x10,a0\n"
			"	movel	0x2c,a2\n"
			"	movel	#1f,d0\n"
			"	movel	d0,0x10\n"
			"	movel	d0,0x2c\n"
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
			"	movel	a2,0x2c\n"
			"	movel	a1,sp"
			: /* no outputs */
			: "d" (old_dtt0), "d" (old_itt0),
			  "d" (old_dtt1), "d" (old_itt1),
			  "d" (old_cacr)
			: "d0", "a0", "a1", "a2" );
	}

	CacheOn = on_flag;
}


static void cache_invalidate( void )
{
	if (!CacheOn)
		return;

	__asm__ __volatile__ (
		/* redirect line-F vector to catch wrong CPU case */
		"	movel	sp,a1\n"
		"	movel	0x2c,a0\n"
		"	movel	#1f,0x2c\n"
		/* '040 and '060: invalidate both caches, no push necessary, since
		   data cache is operated write-through */
		"	.chip	68040\n"
		"	cinva	%%bc\n"
		"	bra		2f\n"
		/* '030: clear both caches */
		"	.chip	68030\n"
		"1:	movec	cacr,d0\n"
		"	orw		#0x0808,d0\n"
		"	movec	d0,cacr\n"
		/* clean up */
		"	.chip	68k\n"
		"2:	movel	a0,0x2c\n"
		"	movel	a1,sp"
		: /* no outputs */
		: /* no inputs */
		: "d0", "a0", "a1" );
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
	if (*p)
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
	cprintf( "Error: %s\n", AlertText[code] );
	exit(-1);
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
long ReadSectors( char *buf, unsigned int _device, unsigned int sector,
				  unsigned int cnt )
{
	int device = _device;
	long rv;
	
	bios_printf( "ReadSectors( dev=%d, sector=%u, cnt=%u, buf=%08lx )\n",
				 device, sector, cnt, (unsigned long)buf );
	if (device < 0) {
		device = -device - 2;
		if (device < 0) device = CurrentFloppy;
		rv = Rwabs( 2, buf, cnt, sector, device );
	}
	else
		rv = DMAread( sector, cnt, buf, device );

	/* invalidate caches after reading something
	 * (DMAread doesn't always use DMA, but it's safer to invalidate the cache
	 * in all cases :-) */
	cache_invalidate();
	return( rv );
}

long WriteSectors( char *buf, int device, unsigned int sector,
				   unsigned int cnt )
{
	bios_printf( "WriteSectors( dev=%d, sector=%u, cnt=%u, buf=%08lx )\n",
				 device, sector, cnt, (unsigned long)buf );
	if (device < 0) {
		device = -device - 2;
		if (device < 0) device = CurrentFloppy;
		return( Rwabs( 3, buf, cnt, sector, device ) );
	}
	else
		return( DMAwrite( sector, cnt, buf, device ) );
	/* no cache maintenance needed for writing, since no write-back cache */
}

#ifdef DEBUG_RW_SECTORS
void bios_printf( const char *format, ... )
{
	static char buf[256];
	char *p;
	
	vsprintf( buf, format, &format+1 );
	for( p = buf; *p; ++p ) {
		if (*p == '\n')
			Bconout( 2, '\r' );
		Bconout( 2, *p );
	}
}
#endif


/* Local Variables: */
/* tab-width: 4     */
/* End:             */
