/*
 * lilo.l.c -- Main file for Atari lilo
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: lilo.l.c,v 1.14 1999-01-18 10:00:54 schwab Exp $
 * 
 * $Log: lilo.l.c,v $
 * Revision 1.14  1999-01-18 10:00:54  schwab
 * (parse_device): Support new scsi disk major numbering.
 *
 * Revision 1.13  1998/07/15 08:26:46  schwab
 * Don't declare WriteLoader here.
 *
 * Revision 1.12  1998/04/06 01:40:59  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.11  1998/03/17 12:31:42  rnhodek
 * Set MaxVectorSector{Number,Count} to limits of DMAread().
 *
 * Revision 1.10  1998/03/10 10:26:01  rnhodek
 * CreateMapFile(): Also test if there are restricted images without a password.
 *
 * Revision 1.9  1998/03/06 09:49:45  rnhodek
 * In CreateBootBlock(), initialize new field 'modif_mask' from SkipOnKeys.
 *
 * Revision 1.8  1998/03/04 09:16:25  rnhodek
 * CreateMapFile: Die if there are no boot records at all; check if
 * kernel file exists only if OSType is BOS_LINUX, and if it isn't a
 * BOOTP image.
 *
 * Revision 1.7  1998/03/03 11:32:33  rnhodek
 * Fixed a bad bug: BootStartSector should always be zero, except for XGM!
 *
 * Revision 1.6  1998/02/23 10:24:48  rnhodek
 * Include bootparam.h for N_MAPENTRIES
 * New function WriteLoaderMap() that patches the map vector of
 * loader.patched into the second (map) sector.
 * Fix pend calculation in {check,recalc}_checksum()
 *
 * Revision 1.5  1998/02/19 20:40:14  rnhodek
 * Make things compile again
 *
 * Revision 1.4  1997/09/19 09:06:56  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.3  1997/08/23 23:09:35  rnhodek
 * New parameter 'set_bootdev' to parse_device
 * Fix handling of XGM partitions
 * Minor output enhancements
 *
 * Revision 1.2  1997/08/12 21:51:07  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:27:09  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/major.h>
#include <scsi/scsi.h>

#include "bootstrap.h"
#include "lilo.h"
#include "lilo_util.h"
#include "parser.h"
#include "writetags.h"
#include "bootparam.h"


/***************************** Prototypes *****************************/

#if 0
static void DumpConf( void );
#endif
static int get_TOS_devicenum( dev_t device );
static void add_dev_cache( const char *name, int devno, unsigned long
                           start, int start_valid, int is_xgm );
static void get_partition_start( const char *device, int is_xgm, unsigned
                                 long *start );
static int get_SCSI_id( const char *device, unsigned int major );
static void get_XGM_sector( const char *device, unsigned long *start );
static void CreateBootBlock( void );
static void WriteLoaderMap( void );
static void CreateMapFile( void);
static int check_checksum( void *buf );
static void recalc_checksum( void *buf );
static void Usage( void);

/************************* End of Prototypes **************************/


#if 0

#define	DUMPL(field)							\
	do {										\
		printf( #field ": " );					\
		if (field)								\
			printf( "%lu\n", *(field) );		\
		else									\
			printf( "NULL\n" );					\
	} while(0)

#define	DUMPX(field)							\
	do {										\
		printf( #field ": " );					\
		if (field)								\
			printf( "%08lx\n", *(field) );		\
		else									\
			printf( "NULL\n" );					\
	} while(0)

#define	DUMPS(field)							\
	do {										\
		printf( #field ": " );					\
		if (field)								\
			printf( "%s\n", field );			\
		else									\
			printf( "NULL\n" );					\
	} while(0)

static void DumpConf( void )
{
	struct BootRecord *br;
	struct FileDef *fd;
	struct TagTmpMnt *mnt;
	int i;
	
	DUMPS( Config.BootDevice );

	DUMPS( Config.Options.Default );
	DUMPL( Config.Options.Auto );
	DUMPL( Config.Options.TimeOut );
	DUMPS( Config.Options.MasterPassword );
	DUMPL( Config.Options.Debug );
	DUMPS( Config.Options.Prompt );
	DUMPL( Config.Options.TmpMnt[0] );
	DUMPL( Config.Options.TmpMnt[1] );
	DUMPL( Config.Options.TmpMnt[2] );
	DUMPL( Config.Options.TmpMnt[3] );
	DUMPS( Config.Options.ExecProg[0] );
	DUMPS( Config.Options.ExecProg[1] );
	DUMPS( Config.Options.ExecProg[2] );
	DUMPS( Config.Options.ExecProg[3] );
	DUMPL( Config.Options.Delay );
	DUMPL( Config.Options.NoGUI );
	DUMPL( Config.Options.Serial );
	DUMPS( Config.Options.SerialParam );
	DUMPL( Config.Options.SerialBaud );
	DUMPL( Config.Options.SerialFlow );
	DUMPL( Config.Options.NoBing );
	DUMPL( Config.Options.VideoRes );
	DUMPX( Config.Options.MCH_cookie );
	DUMPX( Config.Options.CPU_cookie );
	DUMPX( Config.Options.FPU_cookie );

	for( i = 0, br = Config.Records; br; br = br->Next, ++i ) {
		printf( "BOOT RECORD #%d\n", i );
		DUMPS( br->Label );
		DUMPS( br->Alias );
		DUMPS( br->Kernel );
		DUMPS( br->Args );
		DUMPS( br->Password );
		DUMPS( br->Ramdisk );
		DUMPL( br->OSType );
		DUMPL( br->TmpMnt[0] );
		DUMPL( br->TmpMnt[1] );
		DUMPL( br->TmpMnt[2] );
		DUMPL( br->TmpMnt[3] );
		DUMPS( br->ExecProg[0] );
		DUMPS( br->ExecProg[1] );
		DUMPS( br->ExecProg[2] );
		DUMPS( br->ExecProg[3] );
		DUMPL( br->IgnoreTTRam );
		DUMPL( br->LoadToSTRam );
		DUMPL( br->ForceSTRam );
		DUMPL( br->ForceTTRam );
		DUMPL( br->ExtraMemStart );
		DUMPL( br->ExtraMemSize );
		DUMPS( br->TOSDriver );
		DUMPL( br->BootDrv );
		DUMPL( br->BootDev );
		DUMPL( br->BootSec );
	}
	
	for( i = 0, fd = Config.Files; fd; fd = fd->Next, ++i ) {
		printf( "FILE DEF #%d\n", i );
		DUMPS( fd->Path );
		DUMPX( (unsigned long *)fd->Vector );
	}

	for( i = 0, mnt = Config.Mounts; mnt; mnt = mnt->next, ++i ) {
		printf( "MOUNT #%d\n", i );
		DUMPL( mnt->device );
		DUMPL( mnt->start_sec );
		DUMPL( mnt->drive );
		DUMPL( mnt->rw );
	}
}

#endif

static int BootTOSDevno;
static unsigned long BootStartSector;


struct devent {
	struct devent *next;
	char *name;
	int devno;
	unsigned long start;
	unsigned int start_valid : 1;
	unsigned int is_xgm : 1;
} *device_cache = NULL;


void parse_device( char *device, int *devnum, unsigned long *start,
				   int allow_xgm, enum flopok floppy_ok, int set_bootdev )
{
	struct devent *dc;
	struct stat stbuf;
	unsigned int major, minor;
	int is_xgm = 0;
	int len = strlen(device);

	/* first search the cache */
	for( dc = device_cache; dc; dc = dc->next )
		if (strcmp( device, dc->name ) == 0)
			break;
	if (dc) {
		if (!allow_xgm && dc->is_xgm)
			Die( "%s: XGM partition not allowed here\n", device );
		*devnum = dc->devno;
		if (start) {
			if (dc->start_valid)
				*start = dc->start;
			else
				get_partition_start( device, dc->is_xgm, start );
		}
		return;
	}
	
	if (stat( device, &stbuf )) {
		/* if the last letter in the name is x and we can stat the name
		 * without the 'x', we assume the user means an XGM partition */
		int ok = 0;
		if (len > 1 && device[len-1] == 'x') {
			device[len-1] = 0;
			if (stat( device, &stbuf ) == 0)
				ok = is_xgm = 1;
			else
				device[len-1] = 'x';
		}
		if (!ok)
			Die( "Can't stat %s: %s\n", device, strerror(errno) );
	}
	if (!S_ISBLK(stbuf.st_mode))
		Die( "%s is no block special device\n", device );
	major = MAJOR(stbuf.st_rdev);
	minor = MINOR(stbuf.st_rdev);
	if (set_bootdev) {
		/* In case of XGM we have to fake a minor... But it's only used in
		 * CreateBackupFile for the filename. */
		if (is_xgm)
			BootDevice = (major << 8) | 255;
		else
			BootDevice = stbuf.st_rdev;
	}

	/* for SCSI and ACSI devices, use (or try) GET_IDLUN ioctl to get
	 * device ID */
	if (major == ACSI_MAJOR ||
#ifdef SCSI_DISK0_MAJOR
	    SCSI_DISK_MAJOR(major)
#else
	    major == SCSI_DISK_MAJOR
#endif
	    ) {
		*devnum = 8 + get_SCSI_id( device, major );
		if (*devnum == 7)
			/* SCSI_IOCTL_GET_IDLUN wasn't supported by ACSI device */
			goto acsi_major;
		if (major == ACSI_MAJOR)
			*devnum -= 8;
	}
	else if (major == ACSI_MAJOR) {
	  acsi_major:
		/* With ACSI, it's fairly safe to guess the bus ID from the minor
		 * number, because ACSI IDs should be consecutive without any holes. */
		*devnum = (minor >> 4) & 0x0f;
		printf( "Warning: Guessing ACSI ID %d for device %s\n",
				*devnum, device );
	}
	else if (major == HD_MAJOR) {
		/* For IDE, simply driver bus ID from the minor */
		*devnum = 16 + ((minor >> 6) & 3);
	}
	else {
		/* For all other majors, assume floppy disk. Either a real floppy, or
		 * some other block device (loopback or the like) on which a floppy
		 * image is constructed */
		if (floppy_ok == NO_FLOPPY)
			Die( "%s: floppy devices or floppy images not allowed here\n",
				 device );

		/* The loader interprets -1 as the current floppy, i.e. the one the
		 * loader itself was booted from. -2 is floppy A: and -3 is B: */
		if (major == FLOPPY_MAJOR) {
			/* is CUR_FLOPPY, always use -1 for current floppy */
			*devnum = (floppy_ok == CUR_FLOPPY) ? -1 : -2 - (minor & 3);
		}
		else {
			/* Floppy images only allowed with CUR_FLOPPY */
			if (floppy_ok != CUR_FLOPPY)
				Die( "Bad device type (major %u)\n", major );
			printf( "Warning: Assuming that %s contains a floppy image!\n",
					device );
			*devnum = -1;
		}

		/* floppy devices don't have partitions, don't need Geometry */
		*start = 0;
		/* return here, don't add it to the cache */
		return;
	}

	if (start)
		get_partition_start( device, is_xgm, start );
	add_dev_cache( device, *devnum, start ? *start : 0, start!=NULL, is_xgm );
}


static int get_TOS_devicenum( dev_t device )
{
    char devname[PATH_MAX+1];
	int tos_devnum;

	FindDevice( device, devname );
	/* XXX CUR_FLOPPY or ANY_FLOPPY ?? */
	parse_device( devname, &tos_devnum, NULL, 0, CUR_FLOPPY, 0 );
	return( tos_devnum );
}


static void add_dev_cache( const char *name, int devno, unsigned long start,
						   int start_valid, int is_xgm )
{
	struct devent *dc;

	if (!(dc = malloc( sizeof(struct devent) )))
		Error_NoMemory();
	dc->next = device_cache;
	device_cache = dc;

	if (!(dc->name = strdup( name )))
		Error_NoMemory();
	dc->devno       = devno;
	dc->start       = start;
	dc->start_valid = start_valid;
	dc->is_xgm      = is_xgm;
}

/* Determine start sector of partition (maybe also XGM) */
static void get_partition_start( const char *device, int is_xgm,
								 unsigned long *start )
{
	if (!is_xgm)
		GeometryDevice( device, start );
	else
		get_XGM_sector( device, start );
}


static int get_SCSI_id( const char *device, unsigned int major )
{
	unsigned int idlun[2], id, lun;
	int fd;

	if ((fd = open( device, O_RDONLY )) < 0)
		Error_Open( device );
	if (ioctl( fd, SCSI_IOCTL_GET_IDLUN, idlun )) {
		close( fd );
		if (major == ACSI_MAJOR)
			/* not all kernel versions support the ioctl for ACSI, so
			 * don't complain and go to default ACSI case */
			return( -1 );
		Error_Ioctl( device, "SCSI_IOCTL_GET_IDLUN" );
	}
	close( fd );

	id = idlun[0] & 0xff;
	lun = (idlun[0] >> 8) & 0xff;

	if (id > 7)
		Die( "%s is SCSI ID %u, but TOS only supports IDs up to 7.\n",
			 device, id );
	if (lun != 0)
		Die( "%s is SCSI ID %u/LUN %u, but LUN!=0 not supported by TOS\n",
			 device, id, lun );
	return( id );
}


static void get_XGM_sector( const char *device, unsigned long *start )
{
	int fd;
	struct BootBlock rs;
	struct partition *pi;

	/* read the root sector */
	if ((fd = open( device, O_RDONLY )) < 0)
		Error_Open( device );
	if (read( fd, &rs, sizeof(rs) ) != sizeof(rs))
		Error_Read( device );
	close( fd );

	/* search for a valid partition entry with ID "XGM" */
	for( pi = &rs.part[0]; pi < &rs.part[4]; ++pi ) {
		if ((pi->flag & 1) && memcmp( pi->id, "XGM", 3 ) == 0) {
			*start = pi->start;
			return;
		}
	}
	Die( "No XGM partition found on %s\n", device );
}


static void CreateBootBlock( void )
{
    unsigned long map_sector;
	struct BootBlock template;
	int fd;
	
    if (Verbose)
		puts("Creating boot block");

    if ((fd = open( LoaderTemplate, O_RDONLY )) < 0)
		Error_Open( LoaderTemplate );
    if (read( fd, &template, sizeof(struct BootBlock)) != sizeof(struct BootBlock))
		Error_Read( LoaderTemplate );
	close( fd );

	/* determine sector number of map sector: if the first map entry of the
	 * loader file has length 1, it's the start of the second entry, otherwise
	 * it's the sector after the start of the first chunk */
	map_sector = (LoaderVector[1].length == 1) ?
			     LoaderVector[2].start : LoaderVector[1].start + 1;
	
	BootBlock.jump        = template.jump;
	BootBlock.boot_device = LoaderVector[0].length;
	BootBlock.map_sector  = map_sector;
	BootBlock.modif_mask  = Config.SkipOnKeys ? *Config.SkipOnKeys : 0x08;
	memcpy( BootBlock.data, template.data, sizeof(template.data) );
	memcpy( &BootBlock.LiloID, "LILO", 4 );

	recalc_checksum( &BootBlock );
}


static void WriteLoaderMap( void )
{
    int fh;

	if (LoaderNumBlocks+1 > N_MAPENTRIES)
		Die("Loader vector is too large for one sector (%d entries too "
			"much)\n", LoaderNumBlocks+1-N_MAPENTRIES);
	if (Verbose)
	    printf("%d entries for loader vector blocks, %u entries free\n",
			   LoaderNumBlocks, N_MAPENTRIES-1-LoaderNumBlocks);
	memcpy( LoaderData+HARD_SECTOR_SIZE,
			LoaderVector,
			(LoaderNumBlocks+1)*sizeof(struct vecent) );

	/* We must assume here that writing to an existing file (without extending
	 * it) doesn't change which blocks are allocated for it... */
	if (Verbose)
		printf( "Patching loader vector into %s\n", LoaderFile );
    if ((fh = open(LoaderFile, O_RDWR)) == -1)
		Error_Open(LoaderFile);
	if (lseek( fh, HARD_SECTOR_SIZE, SEEK_SET ) != HARD_SECTOR_SIZE)
		Error_Seek(LoaderFile);
    if (write( fh, LoaderData+HARD_SECTOR_SIZE, HARD_SECTOR_SIZE ) !=
		HARD_SECTOR_SIZE)
		Error_Write(LoaderFile);
    close(fh);
}


static void CreateMapFile(void)
{
    const struct BootRecord *record;
    int fd;

	if (!Config.Records)
		Die("No boot records\n");
	
    /* check a few conditions on boot records */
    for (record = Config.Records; record; record = record->Next) {
		if (EqualStrings(Config.Options.Default, record->Label) ||
			EqualStrings(Config.Options.Default, record->Alias)) {
			if (record->OSType && *record->OSType == BOS_LINUX) {
#ifdef USE_BOOTP
				if (strncmp( record->Kernel, "bootp:", 6 ) != 0) {
#endif
					if ((fd = open( record->Kernel, O_RDONLY )) < 0)
						Die("Kernel image `%s' for default boot record `%s' "
							"does not exist\n", record->Kernel, record->Label);
					else
						close( fd );
#ifdef USE_BOOTP
				}
#endif
			}
		}
		if (record->Restricted && *record->Restricted && !record->Password)
			Die("Boot record `%s' is restricted, but has no password\n",
				record->Label);
    }
    
    WriteTags( MapFile );
}


void CheckVectorDevice( const char *name, dev_t device, struct vecent *vector )
{
	/* TOS device number for file is put into length component of the first
	 * vector element */
	vector[0].length = get_TOS_devicenum( device );
}


#define ROOTSEC_CHECKSUM	0x1234

static int check_checksum( void *buf )
{
    signed short *p = (signed short *)buf;
    signed short *pend = (signed short *)(buf + HARD_SECTOR_SIZE);
	signed short sum;
    
    for( sum = 0; p < pend; ++p )
		sum += *p;
    return( sum == ROOTSEC_CHECKSUM );
}


static void recalc_checksum( void *buf )
{
    signed short *p = (signed short *)buf;
    signed short *pend = (signed short *)(buf + HARD_SECTOR_SIZE) - 1;
	signed short sum;
    
    for( sum = 0; p < pend; ++p )
		sum += *p;
    *pend = ROOTSEC_CHECKSUM - sum;
}


static void Usage(void)
{
    Die( "%s\nUsage: %s [options]\n\n"
		 "Valid options are:\n"
		 "    -h, --help                Display this usage information\n"
		 "    -v, --verbose             Enable verbose mode\n"
		 "    -b, --backup              Force a boot block backup on installation\n"
		 "    -u, --uninstall           Uninstall the boot block\n"
		 "    -w, --root prefix         Use another root directory\n"
		 "    -r, --restore-from file   File to restore the boot block from\n"
		 "    -s, --save-to file        File to save the boot block to\n"
		 "    -d, --device device       Override the block special device\n"
		 "    -C, --config-file file    Use file as config file\n"
		 "    -V, --version             Display version information\n",
		 LiloVersion, ProgramName);
}

int main( int argc, char *argv[] )
{
    const char *device = NULL, *conffile = NULL;
	char *pdevice;				/* device name for printing */
#if 0
	extern int confdebug; confdebug = 1;
#endif
	
    ProgramName = argv[0];
	/* set limits of DMAread(): 21 bit sector number, 8 bit sector count */
	MaxVectorSectorNumber = 0x1fffff;
	MaxVectorSectorCount  = 0xff;

    while (--argc > 0) {
		argv++;
		if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
			Usage();
		else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose"))
			Verbose = 1;
		else if (!strcmp(argv[0], "-b") || !strcmp(argv[0], "--backup"))
			DoBackup = 1;
		else if (!strcmp(argv[0], "-u") || !strcmp(argv[0], "--uninstall"))
			Uninstall = 1;
		else if (!strcmp(argv[0], "-V") || !strcmp(argv[0], "--version"))
			Die(LiloVersion);
		else if (!strcmp(argv[0], "-w") || !strcmp(argv[0], "--root")) {
			if (argc-- > 1 && !Root) {
				Root = argv[1];
				argv++;
			} else
				Usage();
		}
		else if (!strcmp(argv[0], "-r") || !strcmp(argv[0], "--restore-from")){
			if (argc-- > 1 && !RestoreBootBlock) {
				RestoreBootBlock = argv[1];
				argv++;
			} else
				Usage();
		}
		else if (!strcmp(argv[0], "-s") || !strcmp(argv[0], "--save-to")) {
			if (argc-- > 1 && !SaveBootBlock) {
				SaveBootBlock = argv[1];
				argv++;
			} else
				Usage();
		}
		else if (!strcmp(argv[0], "-d") || !strcmp(argv[0], "--device")) {
			if (argc-- > 1 && !device) {
				device = argv[1];
				argv++;
			} else
				Usage();
		}
		else if (!strcmp(argv[0], "-C") || !strcmp(argv[0], "--config-file")) {
			if (argc-- > 1 && !conffile) {
				conffile = argv[1];
				argv++;
			} else
				Usage();
		}
		else
			Usage();
    }
    if (!Uninstall && !RestoreBootBlock && !SaveBootBlock)
        Install = 1;
    else if (DoBackup)
		Die( "You can't specify --backup with --uninstall, --restore-from, "
			 "or --save-to\n" );
    if (RestoreBootBlock && Uninstall)
		Die( "You can't specify both --restore-from and --uninstall\n" );

    if (Verbose)
		printf(LiloVersion);

	ConfigFile = CreateFileName(conffile ? conffile : LILO_CONFIGFILE);
    MapFile = CreateFileName(LILO_MAPFILE);
    LoaderTemplate = CreateFileName(LILO_LOADERTEMPLATE);
    LoaderFile = CreateFileName(LILO_LOADERFILE);
    if (SaveBootBlock)
		SaveBootBlock = CreateFileName(SaveBootBlock);
    if (RestoreBootBlock)
		RestoreBootBlock = CreateFileName(RestoreBootBlock);

    ReadConfigFile();
#if 0
	DumpConf();
#endif
    if (device) {
        if (Verbose)
            printf("Using specified device `%s'\n", device);
        Device = device;
    }

	parse_device( (char *)Device, &BootTOSDevno, &BootStartSector, 1,
				  CUR_FLOPPY, 1 );
	if (MINOR(BootDevice) == 255) {
		if (!(pdevice = malloc( strlen(Device) + 2 )))
			Error_NoMemory();
		strcpy( pdevice, Device );
		strcat( pdevice, "x" );
	}
	else {
		pdevice = (char *)Device;
		BootStartSector = 0;
	}
		
    BackupFile = CreateBackupFileName();

    ReadBootBlock( Device, BootStartSector );
    printf( "%s: ", pdevice );
    printf( "%sroot sector is %sbootable\n",
			BootBlock.LiloID == LILO_ID ? "(created by Lilo), " : "",
			check_checksum( &BootBlock ) ? "" : "not " );
	
    if (SaveBootBlock) {
		WriteFile( SaveBootBlock, &BootBlock, sizeof(BootBlock) );
		printf( "Root sector saved to file `%s'\n", SaveBootBlock );
    }
    if (RestoreBootBlock) {
		ReadBootBlock( RestoreBootBlock, 0 );
		printf( "Root sector read from file `%s'\n", RestoreBootBlock );
    }
    if (Uninstall) {
		ReadBootBlock( BackupFile, 0 );
		printf( "Original root sector read from file `%s'\n", BackupFile );
    }

    if (Install) {
		BackupBootBlock();
		CreateMapFile();
		LoaderSize = ReadFile( LoaderTemplate, (void **)&LoaderData );
		if (!(MapVector = CreateVector( MapFile, &MapNumBlocks )))
			Error_Open(MapFile);
		PatchLoader();
		WriteLoader();
		if (!(LoaderVector = CreateVector( LoaderFile, &LoaderNumBlocks )))
			Error_Open(LoaderFile);
		WriteLoaderMap();
		CreateBootBlock();
    }
	
    if (RestoreBootBlock || Install || Uninstall)
		WriteBootBlock( Device, BootStartSector );
	return( 0 );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
