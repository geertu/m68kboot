/* monitor.c -- Boot monitor in Atari LILO
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: monitor.c,v 1.12 1998-03-16 10:48:00 schwab Exp $
 * 
 * $Log: monitor.c,v $
 * Revision 1.12  1998-03-16 10:48:00  schwab
 * Replace strange %lc format by %c and cast arg instead.
 *
 * Revision 1.11  1998/03/10 10:31:12  rnhodek
 * show_record_info(): also print password protection status.
 * do_boot(): Add missing * before CurrRec->OSType.
 *
 * Revision 1.10  1998/03/05 12:47:09  rnhodek
 * Fix harmless typo ("rw" instead of "ro").
 *
 * Revision 1.9  1998/03/05 12:34:45  rnhodek
 * boot command without arguments boots the current boot record.
 *
 * Revision 1.8  1998/03/05 10:26:39  rnhodek
 * Replace body of list_records() by call to new ListRecords() in
 * bootmain.c; the functionality is needed if the monitor is included or not.
 *
 * Revision 1.7  1998/03/04 09:18:43  rnhodek
 * do_exec: accept option -n for running prg without caches.
 *
 * Revision 1.6  1998/03/02 13:56:47  rnhodek
 * Add #ifdefs for NO_MONITOR
 * Don't call graf_init/deinit() if DontUseGUI is set.
 * New optional second arg to 'exec' (workdir).
 *
 * Revision 1.5  1998/02/26 10:25:12  rnhodek
 * New sub-command 'mounts' of 'list'.
 * Args to isprefix() were swapped, and a "== 0" test was left from
 * previous strcmp usage.
 * Fix output format in misc places.
 * Fix parsing device names and partitions.
 *
 * Revision 1.4  1998/02/25 10:38:02  rnhodek
 * New argument 'doprompt' to read_line().
 *
 * Revision 1.3  1998/02/24 11:21:18  rnhodek
 * Fix typo.
 *
 * Revision 1.2  1997/09/19 09:07:00  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:12  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include "loader_config.h"
#ifndef NO_MONITOR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "loader.h"
#include "menu.h"
#include "tmpmnt.h"
#include "sysvars.h"
#include "linuxboot.h"
#include "version.h"

/* MiNTlib doesn't know strcasecmp */
#define strcasecmp	stricmp
#define strncasecmp	strnicmp

struct command {
    const char *name;
    void (*func)(int argc, const char *argv[]);
    const char *help;
};

static struct BootRecord *CurrRec;

/***************************** Prototypes *****************************/

static int monitor_command( void );
static void exec_command( int argc, const char *argv[], const struct
                          command *commands, unsigned int ncommands );
static void parse_line( char *line, int *argcp, const char **argvp[] );
static void show_help( int argc, const char *argv[] );
static void show_version( int argc, const char *argv[] );
static void show_list( int argc, const char *argv[] );
static void list_records( int argc, const char *argv[] );
static void list_files( int argc, const char *argv[] );
static void list_mount( int argc, const char *argv[] );
static void set_record( int argc, const char *argv[] );
static void show_record_info( int argc, const char *argv[] );
static const char *format_partit( const unsigned long *dev, const unsigned
                                  long *start );
static const char *format_device( unsigned long dev );
static void set_record_field( int argc, const char *argv[] );
static void set_type( int argc, const char *argv[] );
static unsigned long getnum( const char *p );
static void set_kernel( int argc, const char *argv[] );
static void set_ramdisk( int argc, const char *argv[] );
static void set_args( int argc, const char *argv[] );
static void set_ignorettram( int argc, const char *argv[] );
static void set_loadtostram( int argc, const char *argv[] );
static void set_forcestram( int argc, const char *argv[] );
static void set_forcettram( int argc, const char *argv[] );
static void set_extrastart( int argc, const char *argv[] );
static void set_extrasize( int argc, const char *argv[] );
static void set_execprog( int argc, const char *argv[] );
static void set_driver( int argc, const char *argv[] );
static void set_bootdrv( int argc, const char *argv[] );
static void set_device( int argc, const char *argv[] );
static void set_sector( int argc, const char *argv[] );
static void show_partition( int argc, const char *argv[] );
static void show_part_info( unsigned int n, struct partition *pi, unsigned long
                            offset );
static void do_mount( int argc, const char *argv[] );
static void do_umount( int argc, const char *argv[] );
static void do_exec( int argc, const char *argv[] );
static void do_boot( int argc, const char *argv[] );
static int parse_device( const char *p, char endc, const char **endp );
static int parse_partit( const char *p, int *dev, unsigned long *start );
static int isprefix( const char *a, const char *b );

/************************* End of Prototypes **************************/

static const struct command commands[] = {
	{ "help", show_help,
	  "                         Display this help" },
	{ "version", show_version,
	  "                      Display version info" },
	{ "list", show_list,
	  " [records|files|mounts]  List boot records or files" },
	{ "use", set_record,
	  " [<label>]                Set current boot record" },
	{ "info", show_record_info,
	  " [<label>]               Show infos from (curr.) boot record" },
	{ "set", set_record_field,
	  " <var> [<val>]            Set variables of curr. boot record" },
	{ "partition", show_partition,
	  " <device>           Show partition table" },
	{ "mount", do_mount,
	  " <partit> <drv>         Mount TOS drive" },
	{ "umount", do_umount,
	  " [<drv>]               Unmount one (all) TOS drives" },
	{ "exec", do_exec,
	  " [-n] <path> [<workdir>]  Execute a TOS program" },
	{ "boot", do_boot,
	  " [<partit> [<driver>]]   Boot curr. record, bootsector or TOS (with driver)" }
};


void boot_monitor( void )
{
	if (!DontUseGUI)
		graf_deinit();
	cprintf( "\fEntering LILO boot monitor.\n" );
	CurrRec = NULL;
	
	while( monitor_command() )
		;

	if (!DontUseGUI)
		graf_init( NULL );
}


static int monitor_command( void )
{
	char *p;
    int noexit = 1, argc;
    const char **argv;

	cprintf( "lilo> " );
	p = read_line( 0, 0 );
	parse_line( p, &argc, &argv );
	if (argc) {
		if (strcmp( argv[0], "exit" ) == 0)
			noexit = 0;
		else
			exec_command( argc, argv, commands, arraysize(commands) );
		free( argv );
	}

	return( noexit );
}


static void exec_command( int argc, const char *argv[],
						  const struct command *commands,
						  unsigned int ncommands )
{
	int i;

	for( i = 0; i < ncommands; ++i ) {
		if (isprefix( argv[0], commands[i].name )) {
			commands[i].func( argc-1, argv+1 );
			return;
		}
	}
	if (i == ncommands)
		cprintf( "Unknown command %s\n", argv[0] );
}

static void parse_line( char *line, int *argcp, const char **argvp[] )
{
	char *p, quote = 0;
	int i;
	
	/* skip initial whitespace */
	line += strspn( line, " \t" );
	
	/* first determine number of arguments and terminate single args with null
	 * bytes */
	for( p = line, *argcp = 0; *p; ++*argcp ) {
		for( ; *p && (quote || !isspace(*p)); ++p ) {
			if (!quote && (*p == '\'' || *p == '"')) {
				quote = *p;
				memmove( p, p+1, strlen(p) );
			}
			else if (*p == quote) {
				quote = 0;
				memmove( p, p+1, strlen(p) );
			}
		}
		if (*p) {
			*p++ = 0;
			p += strspn( p, " \t" );
		}
	}

	if (!*argcp)
		return;

	if (!(*argvp = malloc( (*argcp+1)*sizeof(char *) ))) {
		cprintf( "Out of memory!\n" );
		*argcp = 0;
		return;
	}

	for( p = line, i = 0; i < *argcp; ++i ) {
		(*argvp)[i] = p;
		p += strlen(p) + 1;
		p += strspn( p, " \t" );
	}
	(*argvp)[*argcp] = NULL;
}


static void show_help( int argc, const char *argv[] )
{
	int i;
	
	cprintf( "LILO boot monitor commands:\n" );
	for( i = 0; i < arraysize(commands); ++i )
		cprintf( "  %s %s\n", commands[i].name, commands[i].help );
	cprintf( "<device> is [ACSI|SCSI|IDE]<id> or FLOP[A|B>]\n" );
	cprintf( "<partit> is <device>:<startsector>\n" );
}


static void show_version( int argc, const char *argv[] )
{
	cprintf( "LILO version " VERSION WITH_BOOTP "\n" );
}


static struct command list_commands[] = {
	{ "records", list_records, NULL },
	{ "files", list_files, NULL },
	{ "mounts", list_mount, NULL },
};

static void show_list( int argc, const char *argv[] )
{
	if (!argc) {
		list_records( 0, NULL );
		list_files( 0, NULL );
		list_mount( 0, NULL );
	}
	else
		exec_command( argc, argv, list_commands, arraysize(list_commands) );
}

static void list_records( int argc, const char *argv[] )
{
	ListRecords();
}


static void list_files( int argc, const char *argv[] )
{
    const struct FileDef *file;

    cprintf( "Files:\n" );
    for( file = Files; file; file = file->Next ) {
		cprintf("  %s (", file->Path );
		if (file->Vector)
			cprintf( "%ld bytes", file->Vector[0].start );
		else if (strncmp( file->Path, "bootp:", 6 ) != 0)
			cprintf( "not available" );
		else
			cprintf( "remote" );
		cprintf( ")\n" );
    }
}


static void list_mount( int argc, const char *argv[] )
{
    cprintf( "Mounts:\n" );
	list_mounts();
}


static void set_record( int argc, const char *argv[] )
{
	const struct BootRecord *rec;
	
	if ((rec = FindBootRecord( argc ? argv[0] : NULL ))) {
		CurrRec = (struct BootRecord *)rec;
		if (argc == 0)
			cprintf( "Default boot record %s is current.\n", CurrRec->Label );
	}
	else
		cprintf( "Non-existant boot record\n" );
}


static void show_record_info( int argc, const char *argv[] )
{
	const struct BootRecord *rec;
	struct tmpmnt *mnt;
	int i;

	rec = argc ? FindBootRecord( argv[0] ) : CurrRec;
	if (!rec) {
		cprintf( "Non-existant boot record\n" );
		return;
	}

	cprintf( "Boot record %s:\n", rec->Label );
	if (rec->Alias)
		cprintf( "Alias                : %s\n", rec->Alias );
	switch( rec->OSType ? *rec->OSType : BOS_LINUX ) {
	  case BOS_LINUX:
		cprintf( "OS type              : Linux\n" );
		cprintf( "Kernel               : %s\n",
				 rec->Kernel ? rec->Kernel : "(none)" );
		cprintf( "Ramdisk              : %s\n",
				 rec->Ramdisk ? rec->Ramdisk : "(none)" );
		cprintf( "Arguments            : %s\n",
				 rec->Args ? rec->Args : "(none)" );
		cprintf( "Password protected   : %s\n",
				 rec->Password ?
				 (rec->Restricted ? "manual command line" : "always") :
				 "no" );
		cprintf( "Ignore TT-RAM        : %s\n",
				 (rec->IgnoreTTRam ? *rec->IgnoreTTRam : ignore_ttram) ?
				 "yes" : "no" );
		cprintf( "Load kernel to ST-RAM: %s\n",
				 (rec->LoadToSTRam ? *rec->LoadToSTRam : load_to_stram) ?
				 "yes" : "no" );
		cprintf( "Force ST-RAM size to : " );
		if (rec->ForceSTRam)
			cprintf( "%lu kByte\n", *rec->ForceSTRam / 1024 );
		else
			cprintf( "(no force)\n" );
		cprintf( "Force TT-RAM size to : " );
		if (rec->ForceTTRam)
			cprintf( "%lu kByte\n", *rec->ForceTTRam / 1024 );
		else
			cprintf( "(no force)\n" );
		cprintf( "Extra memory block   : start " );
		if (rec->ExtraMemStart)
			cprintf( "0x%08lx", *rec->ExtraMemStart );
		else
			cprintf( "(undefined)" );
		cprintf( ", size " );
		if (rec->ExtraMemSize)
			cprintf( "%lu kByte", *rec->ExtraMemSize / 1024 );
		else
			cprintf( "(undefined)" );
		cprintf( "\n" );
		cprintf( "Temporary mounts     :\n" );
		for( i = 0; i < MAX_TMPMNT; ++i ) {
			if (!rec->TmpMnt[i])
				continue;
			mnt = &MountPoints[*rec->TmpMnt[i]];
			cprintf( "  (%d) %s on %c: (r%c)\n", i,
					 format_partit( (unsigned long *)&mnt->device,
									&mnt->start_sec ),
					 mnt->drive + 'A', mnt->rw ? 'w' : 'o' );
		}
		cprintf( "Programs to execute  :\n" );
		for( i = 0; i < MAX_EXECPROG; ++i ) {
			if (rec->ExecProg[i])
				cprintf( "  (%d) %s\n", i, rec->ExecProg[i] );
		}
		break;
		
	  case BOS_TOS:
		cprintf( "OS type              : TOS\n" );
		cprintf( "HD driver name       : %s\n",
				 rec->TOSDriver ? rec->TOSDriver : "(none)" );
		cprintf( "Boot drive setting   : \n" );
		if (rec->BootDrv)
			cprintf( "%c:\n", (int)*rec->BootDrv + 'A' );
		else
			cprintf( "(undefined)\n" );
		goto tos_bootb_common;
		
	  case BOS_BOOTB:
		cprintf( "OS type              : boot sector\n" );
	  tos_bootb_common:
		cprintf( "Boot drive/partition : %s\n",
				 format_partit( rec->BootDev, rec->BootSec ) );
		break;
		
	  default:
		printf( "OS type               : unknown (%lu)\n", *rec->OSType );
		break;
	}
}


static const char *format_partit( const unsigned long *dev,
								  const unsigned long *start )
{
	static char buf[26];

	sprintf( buf, "%s:", dev ? format_device( *dev ) : "undef" );
	if (start)
		sprintf( buf+strlen(buf), "%lu", *start );
	else
		strcat( buf, "undef" );
	return( buf );
}


static const char *format_device( unsigned long dev )
{
	static char buf[16];

	if (dev < 0 && dev >= -3)
		sprintf( buf, "FLOP%s", dev == -2 ? "A" : dev == -3 ? "B" : "" );
	else if (dev < 8)
		sprintf( buf, "ACSI%ld", dev );
	else if (dev < 16)
		sprintf( buf, "SCSI%ld", dev-8 );
	else if (dev < 18)
		sprintf( buf, "IDE%ld", dev-16 );
	else
		sprintf( buf, "bad(%ld)", dev );
	return( buf );
}


static struct command set_commands[] = {
	{ "type", set_type, NULL },
	{ "kernel", set_kernel, NULL },
	{ "ramdisk", set_ramdisk, NULL },
	{ "args", set_args, NULL },
	{ "ignorettram", set_ignorettram, NULL },
	{ "loadtostram", set_loadtostram, NULL },
	{ "forcestram", set_forcestram, NULL },
	{ "forcettram", set_forcettram, NULL },
	{ "extrastart", set_extrastart, NULL },
	{ "extrasize", set_extrasize, NULL },
	{ "execprog", set_execprog, NULL },
	{ "driver", set_driver, NULL },
	{ "bootdrv", set_bootdrv, NULL },
	{ "device", set_device, NULL },
	{ "sector", set_sector, NULL },
};

static void set_record_field( int argc, const char *argv[] )
{
	if (!CurrRec) {
		cprintf( "No current boot record defined.\n" );
		return;
	}
	if (!argc)
		cprintf( "Missing arguments\n" );
	else
		exec_command( argc, argv, set_commands, arraysize(set_commands) );
}


static void set_type( int argc, const char *argv[] )
{
	if (!argc)
		cprintf( "OS type missing\n" );
	else if (isprefix( argv[0], "linux" ))
		*(unsigned long *)CurrRec->OSType = BOS_LINUX;
	else if (isprefix( argv[0], "tos" ))
		*(unsigned long *)CurrRec->OSType = BOS_TOS;
	else if (isprefix( argv[0], "bootsector" ))
		*(unsigned long *)CurrRec->OSType = BOS_BOOTB;
	else
		cprintf( "Bad OS type\n" );
}

#define	SET_STR(field) \
	CurrRec->field = (argc ? strdup(argv[0]) : NULL);
#define	PREPARE_FIELD(field)								\
	if (!argc) {											\
		CurrRec->field = NULL;								\
		return;												\
	}														\
	if (!CurrRec->field)									\
		CurrRec->field = malloc( sizeof(unsigned long) );
#define	SET_NUM(field)							\
	PREPARE_FIELD(field);						\
	*(unsigned long *)CurrRec->field = getnum(argv[0]);

static unsigned long getnum( const char *p )
{
    unsigned long val;
    char *end;
    
    val = strtoul( p, &end, 0 );
    if (end == p) {
		cprintf( "Invalid number: %s\n", p );
		return( 0 );
    }

    if (islower(*end) == 'k')
		val *= 1024;
    else if (islower(*end) == 'm')
		val *= 1024*1024;
    return( val );
}

static void set_kernel( int argc, const char *argv[] )
{ SET_STR( Kernel ); }

static void set_ramdisk( int argc, const char *argv[] )
{ SET_STR( Ramdisk ); }

static void set_args( int argc, const char *argv[] )
{ SET_STR( Args ); }

static void set_ignorettram( int argc, const char *argv[] )
{ SET_NUM( IgnoreTTRam ); }

static void set_loadtostram( int argc, const char *argv[] )
{ SET_NUM( LoadToSTRam ); }

static void set_forcestram( int argc, const char *argv[] )
{ SET_NUM( ForceSTRam ); }

static void set_forcettram( int argc, const char *argv[] )
{ SET_NUM( ForceTTRam ); }

static void set_extrastart( int argc, const char *argv[] )
{ SET_NUM( ExtraMemStart ); }

static void set_extrasize( int argc, const char *argv[] )
{ SET_NUM( ExtraMemSize ); }

static void set_execprog( int argc, const char *argv[] )
{
	int i;
	
	if (!argc) {
		cprintf( "Missing index\n" );
		return;
	}
	i = atoi(argv[0] );
	--argc; ++argv;
	SET_STR( ExecProg[i] );
}

static void set_driver( int argc, const char *argv[] )
{ SET_STR( TOSDriver ); }

static void set_bootdrv( int argc, const char *argv[] )
{
	PREPARE_FIELD(BootDrv);
	if (isalpha(argv[0][0]))
		*(unsigned long *)CurrRec->BootDrv = toupper(argv[0][0]) - 'A';
	else
		*(unsigned long *)CurrRec->BootDrv = getnum( argv[0] );
}

static void set_device( int argc, const char *argv[] )
{
	PREPARE_FIELD(BootDrv);
	*(unsigned long *)CurrRec->BootDev = parse_device( argv[0], 0, NULL );
}

static void set_sector( int argc, const char *argv[] )
{ SET_NUM( BootSec ); }



static void show_partition( int argc, const char *argv[] )
{
	struct BootBlock *rs = (struct BootBlock *)_dskbuf;
	struct BootBlock *xrs = rs + 1;
	struct partition *pi;
	unsigned int num = 0;
	unsigned long sec, extoffset;
	int dev;
	long err;
	
	if (!argc) {
		cprintf( "Missing device\n" );
		return;
	}
	if ((dev = parse_device( argv[0], 0, NULL )) == 9999)
		return;

	cprintf( "part. ID  boot     start      size\n" );
	if ((err = ReadSectors( (char *)rs, dev, 0, 1 ))) {
		printf( "Error reading sector 0: %s\n", tos_perror(err) );
		return;
	}
	for( pi = &rs->part[0]; pi < &rs->part[4]; ++pi ) {
		if (!(pi->flag & 1))
			continue;
		if (memcmp( pi->id, "XGM", 3 ) == 0) {
			sec = extoffset = pi->start;
			do {
				if ((err = ReadSectors( (char *)xrs, dev, sec, 1 ))) {
					printf( "Error reading sector %lu: %s\n", sec,
							tos_perror(err) );
					return;
				}
				if (xrs->part[0].flag & 1)
					show_part_info( ++num, &xrs->part[0], sec );
			} while( (xrs->part[1].flag & 1) &&
					 memcmp( xrs->part[1].id, "XGM", 3 ) == 0 &&
					 (sec = xrs->part[1].start + extoffset) );
		}
		else
			show_part_info( ++num, pi, 0 );
	}
}

static void show_part_info( unsigned int n, struct partition *pi,
							unsigned long offset )
{
	cprintf( "  %2u: %c%c%c $%02x  %9lu %9lu%s kByte\n",
			 n,
			 pi->id[0], pi->id[1], pi->id[2],
			 pi->flag & ~1,
			 pi->start + offset,
			 pi->size / 2,
			 (pi->size & 1) ? "+" : "" );
}


static void do_mount( int argc, const char *argv[] )
{
	struct tmpmnt mnt;

	if (argc < 2) {
		cprintf( "Need at least two arguments\n" );
		return;
	}
	if (!parse_partit( argv[0], &mnt.device, &mnt.start_sec )) {
		cprintf( "Bad partition\n" );
		return;
	}
	
	if (!isalpha(argv[1][0]) ||
		!(!argv[1][1] || (argv[1][1] == ':' && !argv[1][2]))) {
		cprintf( "Bad drive\n" );
	}
	mnt.drive = toupper(argv[1][0]) - 'A';

	mnt.rw = 0;
	if (argc >= 3) {
		if (strcasecmp( argv[2], "rw" ) == 0)
			mnt.rw = 1;
		else if (strcasecmp( argv[2], "ro" ) == 0)
			;
		else {
			cprintf( "Third arg must be 'ro' or 'rw'\n" );
			return;
		}
	}

	if (!mount( &mnt ))
		cprintf( "Mount failed.\n" );
}


static void do_umount( int argc, const char *argv[] )
{
	if (!argc) {
		umount();
		return;
	}
	
	if (!isalpha(argv[0][0]) ||
		!(!argv[0][1] || (argv[0][1] == ':' && !argv[0][2]))) {
		cprintf( "Bad drive\n" );
	}
	umount_drv( toupper(argv[0][0]) - 'A' );
}


static void do_exec( int argc, const char *argv[] )
{
	int rv;
	int use_cache = 1;
	
	if (!argc) {
		cprintf( "Need program to execute\n" );
		return;
	}
	if (strcmp( argv[0], "-n" ) == 0) {
		use_cache = 0;
		--argc;
		++argv;
	}
	if (argc > 2) {
		cprintf( "Write arguments to command into first arg (with quotes)\n" );
		cprintf( "(Second arg is workdir!)\n" );
		return;
	}

	rv = exec_tos_program( argv[0], argc == 2 ? argv[1] : NULL, use_cache );
	cprintf( "Return value: %d\n", rv );
}


static void do_boot( int argc, const char *argv[] )
{
	struct BootRecord rec;
	int dev;
	unsigned long sec, drv;
	
	if (!argc) {
		/* boot current boot record */
		if (!CurrRec) {
			cprintf( "No current boot record.\n" );
			return;
		}
		if (!CurrRec->OSType) {
			cprintf( "OS type not defined in current boot record!\n" );
			return;
		}
		switch( *CurrRec->OSType ) {
		  case BOS_TOS:
			boot_tos( CurrRec );
			break;
		  case BOS_LINUX:
			boot_linux( CurrRec, "" );
			break;
		  case BOS_BOOTB:
			boot_bootsector( CurrRec );
			break;
		  default:
			cprintf( "Undefined OS type %lu\n", *CurrRec->OSType );
		}
		return;
	}

	rec.BootDev = rec.BootSec = rec.BootDrv = NULL;
	rec.TOSDriver = NULL;
	if (!parse_partit( argv[0], &dev, &sec )) {
		cprintf( "Bad partition\n" );
		return;
	}
	rec.BootDev = (unsigned long *)&dev;
	rec.BootSec = &sec;
	
	if (argc > 1)
		rec.TOSDriver = argv[1];

	if (argc > 2) {
		if (!isalpha(argv[2][0]) ||
			!(!argv[2][1] || (argv[2][1] == ':' && !argv[2][2]))) {
			cprintf( "Bad drive\n" );
		}
		drv = toupper(argv[2][0]) - 'A';
		rec.BootDrv = &drv;
	}
					  
	if (rec.TOSDriver)
		boot_tos( &rec );
	else
		boot_bootsector(  &rec );
}


static int parse_device( const char *p, char endc, const char **endp )
{
	int dev;
	
	if (strncasecmp( p, "ACSI", 4 ) == 0)
		dev = 0, p += 4;
	else if (strncasecmp( p, "SCSI", 4 ) == 0)
		dev = 8, p += 4;
	else if (strncasecmp( p, "IDE", 3 ) == 0)
		dev = 16, p += 3;
	else if (strncasecmp( p, "FLOP", 4 ) == 0)
		dev = -1, p += 4;
	else {
	  bad:
		cprintf( "Bad device\n" );
		return( 9999 );
	}

	if (dev >= 0) {
		if (!isdigit(*p) || *p > (dev == 16 ? '1' : '7') || p[1] != endc)
			goto bad;
		dev += *p - '0';
		++p;
	}
	else {
		if (*p != endc) {
			if ((*p != 'A' && *p != 'B') || p[1] != endc)
				goto bad;
			dev -= *p - 'A' + 1;
			++p;
		}
	}
	if (endp)
		*endp = p;
	return( dev );
}


static int parse_partit( const char *p, int *dev, unsigned long *start )
{
	if ((*dev = parse_device( p, ':', &p )) == 9999)
		return( 0 );
	if (*p != ':') {
		cprintf( "Missing ':' after device name\n" );
		return( 0 );
	}
	*start = getnum( p+1 );
	return( 1 );
}


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

#endif /* NO_MONITOR */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
