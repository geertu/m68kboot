/*
 * lilo.l.c -- Main file for Atari lilo
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: lilo.l.c,v 1.2 1997-08-12 21:51:07 rnhodek Exp $
 * 
 * $Log: lilo.l.c,v $
 * Revision 1.2  1997-08-12 21:51:07  rnhodek
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


/***************************** Prototypes *****************************/

static void DumpConf( void );
static int get_TOS_devicenum( dev_t device );
static void add_dev_cache( const char *name, int devno, unsigned long
                           start, int start_valid, int is_xgm );
static void get_partition_start( const char *device, int is_xgm, unsigned
                                 long *start );
static int get_SCSI_id( const char *device, unsigned major );
static void get_XGM_sector( const char *device, unsigned long *start );
static void CreateBootBlock( void );
static void CreateMapFile( void);
static int check_checksum( void *buf );
static void recalc_checksum( void *buf );
static void WriteLoader( void);
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
	unsigned start_valid : 1;
	unsigned is_xgm : 1;
} *device_cache = NULL;


void parse_device( const char *device, int *devnum, unsigned long *start,
				   int allow_xgm, enum flopok floppy_ok )
{
	struct devent *dc;
	struct stat stbuf;
	unsigned major, minor;
	int is_xgm = 0;
	int len = strlen(device);
	char dev2[len];

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
			strncpy( dev2, device, len-1 );
			dev2[len] = 0;
			if (stat( dev2, &stbuf ) == 0)
				ok = is_xgm = 1;
		}
		if (!ok)
			Die( "Can't stat %s: %s\n", device, strerror(errno) );
	}
	if (!S_ISBLK(stbuf.st_mode))
		Die( "%s is no block special device\n", device );
	major = MAJOR(stbuf.st_rdev);
	minor = MINOR(stbuf.st_rdev);

	/* for SCSI and ACSI devices, use (or try) GET_IDLUN ioctl to get
	 * device ID */
	if (major == SCSI_DISK_MAJOR || major == ACSI_MAJOR) {
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
	parse_device( devname, &tos_devnum, NULL, 0, CUR_FLOPPY );
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
	else {
		int len = strlen(device);
		char dev2[len+1];
		strncpy( dev2, device, len-1 );
		dev2[len] = 0;
		get_XGM_sector( dev2, start );
	}
}


static int get_SCSI_id( const char *device, unsigned major )
{
	unsigned idlun[2], id, lun;
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
	memcpy( BootBlock.data, template.data, sizeof(template.data) );
	memcpy( &BootBlock.LiloID, "LILO", 4 );

	recalc_checksum( &BootBlock );
}


static void CreateMapFile(void)
{
    const struct BootRecord *record;
    int fd;
    
    /* check a few conditions on the default boot record */
    for (record = Config.Records; record; record = record->Next) {
		if (EqualStrings(Config.Options.Default, record->Label) ||
			EqualStrings(Config.Options.Default, record->Alias)) {
			if ((fd = open( record->Kernel, O_RDONLY )) < 0)
				Die("Kernel image `%s' for default boot record `%s' does not "
					"exist\n", record->Kernel, record->Label);
			else
				close( fd );
			break;
		}
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
    signed short *pend = buf + (HARD_SECTOR_SIZE/sizeof(short));
	signed short sum;
    
    for( sum = 0; p < pend; ++p )
		sum += *p;
    return( sum == ROOTSEC_CHECKSUM );
}


static void recalc_checksum( void *buf )
{
    signed short *p = (signed short *)buf;
    signed short *pend = buf + (HARD_SECTOR_SIZE/sizeof(short) - 1);
	signed short sum;
    
    for( sum = 0; p < pend; ++p )
		sum += *p;
    *pend = ROOTSEC_CHECKSUM - sum;
}


    /*
     *  Write the Loader (incl. Header Code)
     */

static void WriteLoader(void)
{
    int fh;

    if (Verbose)
		printf( "Writing loader to file `%s'\n", LoaderFile );

    if ((fh = open(LoaderFile, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
		Error_Open(LoaderFile);
    if (write( fh, LoaderData, LoaderSize ) != LoaderSize)
		Error_Write(LoaderFile);
    close(fh);
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
    struct stat info;
    const char *device = NULL, *conffile = NULL;
#if 0
	extern int confdebug; confdebug = 1;
#endif
	
    ProgramName = argv[0];

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
		else if (!strcmp(argv[0], "-w") || !strcmp(argv[0], "--root"))
			if (argc-- > 1 && !Root) {
				Root = argv[1];
				argv++;
			} else
				Usage();
		else if (!strcmp(argv[0], "-r") || !strcmp(argv[0], "--restore-from"))
			if (argc-- > 1 && !RestoreBootBlock) {
				RestoreBootBlock = argv[1];
				argv++;
			} else
				Usage();
		else if (!strcmp(argv[0], "-s") || !strcmp(argv[0], "--save-to"))
			if (argc-- > 1 && !SaveBootBlock) {
				SaveBootBlock = argv[1];
				argv++;
			} else
				Usage();
		else if (!strcmp(argv[0], "-d") || !strcmp(argv[0], "--device"))
			if (argc-- > 1 && !device) {
				device = argv[1];
				argv++;
			} else
				Usage();
		else if (!strcmp(argv[0], "-C") || !strcmp(argv[0], "--config-file"))
			if (argc-- > 1 && !conffile) {
				conffile = argv[1];
				argv++;
			} else
				Usage();
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

    if (stat(Device, &info) == -1)
		Error_Stat(Device);
    if (!S_ISBLK(info.st_mode))
		Die("%s is not a block special device\n", Device);
    BootDevice = info.st_rdev;
	parse_device( Device, &BootTOSDevno, &BootStartSector, 1, CUR_FLOPPY );
    BackupFile = CreateBackupFileName();

    ReadBootBlock( Device, BootStartSector );
    printf( "%s: ", Device );
    printf( "%s, root sector is %sbootable\n",
			BootBlock.LiloID == LILO_ID ? " (created by Lilo)" : "",
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
		CreateBootBlock();
    }
	
    if (RestoreBootBlock || Install || Uninstall)
		WriteBootBlock( Device, BootStartSector );
	return( 0 );
}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
