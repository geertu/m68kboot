/*
 *  Amiga Linux/m68k Loader -- Main Program
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  The original idea for this program comes from the Linux distribution:
 *
 *	LILO - Generic Boot Loader for Linux ("LInux LOader")
 *	       by Werner Almesberger
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: lilo.l.c,v 1.9 2000-06-15 18:39:32 dorchain Exp $
 * 
 * $Log: lilo.l.c,v $
 * Revision 1.9  2000-06-15 18:39:32  dorchain
 * Checksumm inline asm was overoptimized by gcc
 *
 * Revision 1.8  2000/06/04 17:14:55  dorchain
 * Fixed compile errors.
 * it still doesn't work for me
 *
 * Revision 1.7  1999/01/18 10:02:10  schwab
 * (CheckVectorDevice): Support new scsi disk major
 * numbering.
 *
 * Revision 1.6  1998/04/06 01:40:57  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.5  1998/03/31 11:40:46  rnhodek
 * Replaced sizeof(u_long) by sizeof(struct vecent) in calculations based
 * on LoaderNumBlocks.
 *
 * Revision 1.4  1998/03/04 09:12:53  rnhodek
 * CreateMapFile: Die if there are no boot records at all; check if
 * kernel file exists only if OSType is BOS_LINUX, and if it isn't a
 * BOOTP image.
 *
 * Revision 1.3  1998/02/19 21:07:08  rnhodek
 * Added if/else braces to avoid gcc warnings
 *
 * Revision 1.2  1997/08/12 21:51:05  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:27:04  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <linux/elf.h>
#include <linux/major.h>
#include <linux/hdreg.h>

#include "linuxboot.h"
#include "lilo.h"
#include "lilo_util.h"
#include "config.h"
#include "parser.h"
#include "writetags.h"


    /*
     *	Command Line Options
     */

static int Force = 0;
static int DoChecksum = 0;
static int DoCreateReset = 0;
static const char *DosType = NULL;


    /*
     *	Boot Code Definitions
     */

extern u_char _BootCodeStart[];
extern u_char _BootCodeEnd[];
extern u_char _LoaderVectorStart[];
extern u_char _LoaderVectorEnd[];
extern u_char _AltDeviceName[];
extern u_char _AltDeviceUnit[];


    /*
     *	Reset Code Definitions
     */

extern u_char _ResetStart[];
extern u_char _ResetEnd[];


    /*
     *	Valid DosTypes
     */

struct DosTypeEntry {
    u_long ID;
    const char *ASCII;
    const char *Description;
};

static struct DosTypeEntry DosTypeEntries[] = {
    { ID_BOOT_DISK, "BOOU", "Generic boot disk" },
    { ID_OFS_DISK, "DOS0", "OFS disk" },
    { ID_FFS_DISK, "DOS1", "FFS disk" },
    { ID_OFS_INTL_DISK, "DOS2", "OFS INTL disk" },
    { ID_FFS_INTL_DISK, "DOS3", "FFS INTL disk" },
    { ID_OFS_DC_DISK, "DOS4", "OFS DC disk" },
    { ID_FFS_DC_DISK, "DOS5", "FFS DC disk" },
    { ID_MUFS_DISK, "muFS", "MultiUser FFS INTL disk" },
    { ID_MU_OFS_DISK, "muF0", "MultiUser OFS disk" },
    { ID_MU_FFS_DISK, "muF1", "MultiUser FFS disk" },
    { ID_MU_OFS_INTL_DISK, "muF2", "MultiUser OFS INTL disk" },
    { ID_MU_FFS_INTL_DISK, "muF3", "MultiUser FFS INTL disk" },
    { ID_MU_OFS_DC_DISK, "muF4", "MultiUser OFS DC disk" },
    { ID_MU_FFS_DC_DISK, "muF5", "MultiUser FFS DC disk" },
    { 0, NULL, NULL }
};


    /*
     *	Amiga Specific Lilo Data
     */


static u_long DosTypeID;


    /*
     *	Function Prototypes
     */

int main(int argc, char *argv[]);

static void CreateBootBlock(int reset);
static void SetDosType(void);
static void FixChecksum(void);
static u_long CalcChecksum(void);
static void CreateMapFile(void);
static void Usage(void) __attribute__ ((noreturn));


    /*
     *	Create the Boot Block
     */

static void CreateBootBlock(int reset)
{
    u_long start, size, maxsize, data;

    if (Verbose)
	puts("Creating boot block");

    if (reset) {
	start = (u_long)_ResetStart;
	size = (u_long)_ResetEnd;
    } else {
	start = (u_long)_BootCodeStart;
	size = (u_long)_BootCodeEnd;
    }
    size -= start;
    data = (u_long)BootBlock.Data;
    maxsize = sizeof(BootBlock.Data);
    memset((void *)data, 0, maxsize);
    if (size > maxsize)
	Die("%ld bytes too much boot code\n", size-maxsize);
    if (Verbose)
	printf("%lu bytes boot code, %lu bytes free\n", size, maxsize-size);
    BootBlock.ID = DosTypeID;
    if (BootBlock.ID == ID_BOOT_DISK)
	BootBlock.DosBlock = 0;
    memcpy((void *)data, (void *)start, size);

    if (!reset) {
	maxsize = (_LoaderVectorEnd-_LoaderVectorStart)/sizeof(struct vecent)-1;
	if (LoaderNumBlocks > maxsize)
	    Die("Loader vector is too large for boot block (%ld entries too "
		"much)\n", LoaderNumBlocks-maxsize);
	if (Verbose)
	    printf("%d entries for loader vector blocks, %lu entries free\n",
		   LoaderNumBlocks, maxsize-LoaderNumBlocks);
	memcpy((void *)(data+(u_long)_LoaderVectorStart-start), LoaderVector,
	       (LoaderNumBlocks+1)*sizeof(struct vecent));
	if (Config.AltDeviceName) {
	    strcpy((void *)(data+(u_long)_AltDeviceName-start),
		   Config.AltDeviceName);
	    memcpy((void *)(data+(u_long)_AltDeviceUnit-start),
		   Config.AltDeviceUnit, sizeof(Config.AltDeviceUnit));
	}
    }
    BootBlock.LiloID = LILO_ID;
    FixChecksum();
}


    /*
     *	Set the DosType of the Boot Block
     */

static void SetDosType(void)
{
    int i;

    for (i = 0; DosTypeEntries[i].ID; i++)
	if (!strcmp(DosType, DosTypeEntries[i].ASCII))
	    break;
    if (DosTypeEntries[i].ID)
	DosTypeID = DosTypeEntries[i].ID;
    else {
	printf("Unknown DosType `%s'\n", DosType);
	puts("Valid DosTypes are:");
	for (i = 0; DosTypeEntries[i].ID; i++)
	    printf(" %s", DosTypeEntries[i].ASCII);
	Die("\n");
    }
    BootBlock.ID = DosTypeID;
    if (Verbose)
	printf("DosType set to `%s'\n", DosType);
    FixChecksum();
}


    /*
     *	Fix the Checksum of the Boot Block
     */

static void FixChecksum(void)
{
    if (Verbose)
	puts("Calculating boot block checksum");

    BootBlock.Checksum = 0;
    BootBlock.Checksum = CalcChecksum();
}


    /*
     *	Calculate the Boot Block Checksum
     */

static u_long CalcChecksum(void)
{
    u_long _null, _res ; /* to avoid gcc optimizer collapsing registers */
    struct BootBlock *bb = &BootBlock;

    __asm __volatile ("1: addl %0@+,%1;"
		      "   addxl %2,%1;"
		      "   dbf %5,1b;"
		      : "=a" (bb), "=d" (_res), "=d" (_null)
		      : "0" (bb), "1" (0), "d" (255), "2" (0));
    return(~_res);
}


    /*
     *	Create the Map File
     */

static void CreateMapFile(void)
{
    const struct BootRecord *record;
    int fd;
    
    if (!Config.Records)
	Die("No boot records\n");
	
    /* check a few conditions on the default boot record */
    for (record = Config.Records; record; record = record->Next) {
	if (EqualStrings(Config.Options.Default, record->Label) ||
	    EqualStrings(Config.Options.Default, record->Alias)) {
	    if (record->OSType && *record->OSType != BOS_LINUX)
		break;
	    if (record->Password)
		Die("Default boot record `%s' must not have a password\n",
		    record->Label);
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
	    break;
	}
    }
    
    WriteTags( MapFile );
}


    /*
     *	Callback for CreateVector to Check the Installation Device
     */

void CheckVectorDevice( const char *name, dev_t device, struct vecent *vector )
{
    static u_int first = 1;
    static dev_t altdev;

    if (!Config.AltDeviceName) {
	if (MAJOR(device) != MAJOR(BootDevice))
	    Die("File `%s' must reside on physical device `%s'\n", name,
		Device);
	switch (MAJOR(BootDevice)) {
	    case FLOPPY_MAJOR:
		if (MINOR(device) != MINOR(BootDevice))
		    Die("File `%s' must reside on physical floppy device "
			"`%s'\n",
			name, Device);
		break;
	    case HD_MAJOR:
		if ((MINOR(device) & 0xc0) != (MINOR(BootDevice) & 0xc0))
		    Die("File `%s' must reside on physical IDE device `%s'\n",
			name, Device);
		break;
#ifdef SCSI_DISK0_MAJOR
	    case SCSI_DISK0_MAJOR:
#else
	    case SCSI_DISK_MAJOR:
#endif
		if ((MINOR(device) & 0xf0) != (MINOR(BootDevice) & 0xf0))
		    Die("File `%s' must reside on physical SCSI device `%s'\n",
			name, Device);
		break;
	    default:
		Die("Major device %d not supported (yet)\n", MAJOR(device));
		break;
	}
    } else if (first) {
	altdev = device;
	printf("Assuming `%s' resides on `%s:%ld'\n", name,
	       Config.AltDeviceName, *Config.AltDeviceUnit);
	first = 0;
    } else {
	if (MAJOR(device) != MAJOR(altdev))
	    Die("File `%s' must reside on `%s:%ld'\n", name,
		Config.AltDeviceName, *Config.AltDeviceUnit);
	switch (MAJOR(altdev)) {
	    case FLOPPY_MAJOR:
		if (MINOR(device) != MINOR(altdev))
		    Die("File `%s' must reside on physical floppy device "
			"`%s:%ld'\n",
			name, Config.AltDeviceName, *Config.AltDeviceUnit);
		break;
	    case HD_MAJOR:
		if ((MINOR(device) & 0xc0) != (MINOR(altdev) & 0xc0))
		    Die("File `%s' must reside on physical IDE device "
			"`%s:%ld'\n",
			name, Config.AltDeviceName, *Config.AltDeviceUnit);
		break;
#ifdef SCSI_DISK0_MAJOR
	    case SCSI_DISK0_MAJOR:
#else
	    case SCSI_DISK_MAJOR:
#endif
		if ((MINOR(device) & 0xf0) != (MINOR(altdev) & 0xf0))
		    Die("File `%s' must reside on physical SCSI device "
			"`%s:%ld'\n",
			name, Config.AltDeviceName, *Config.AltDeviceUnit);
		break;
	    default:
		Die("Major device %d not supported (yet)\n", MAJOR(device));
		break;
	}
    }
}

    /*
     *	Print the Usage Template and Exit
     */

static void Usage(void)
{
    Die("%s\nUsage: %s [options]\n\n"
	"Valid options are:\n"
	"    -h, --help                Display this usage information\n"
	"    -v, --verbose             Enable verbose mode\n"
	"    -f, --force               Force installation over an unknown boot block\n"
	"    -b, --backup              Force a boot block backup on installation\n"
	"    -u, --uninstall           Uninstall the boot block\n"
	"    -w, --root prefix         Use another root directory\n"
	"    -r, --restore-from file   File to restore the boot block from\n"
	"    -s, --save-to file        File to save the boot block to\n"
	"    -d, --device device       Override the block special device\n"
	"    -t, --dostype dostype     Dostype for the boot block\n"
	"    -c, --checksum            Fix the boot block checksum\n"
	"    -x, --create-reset        Create a reset boot block\n"
	"    -C, --config-file file    Use file as config file\n"
	"    -V, --version             Display version information\n",
	LiloVersion, ProgramName);
}


    /*
     *	Main Routine
     */

int main(int argc, char *argv[])
{
    struct stat info;
    int i;
    const char *device = NULL, *conffile = NULL;

    ProgramName = argv[0];

    while (--argc > 0) {
	argv++;
	if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
	    Usage();
	else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose"))
	    Verbose = 1;
	else if (!strcmp(argv[0], "-f") || !strcmp(argv[0], "--force"))
	    Force = 1;
	else if (!strcmp(argv[0], "-b") || !strcmp(argv[0], "--backup"))
	    DoBackup = 1;
	else if (!strcmp(argv[0], "-u") || !strcmp(argv[0], "--uninstall"))
	    Uninstall = 1;
	else if (!strcmp(argv[0], "-c") || !strcmp(argv[0], "--checksum"))
	    DoChecksum = 1;
	else if (!strcmp(argv[0], "-x") || !strcmp(argv[0], "--create-reset"))
	    DoCreateReset = 1;
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
	else if (!strcmp(argv[0], "-t") || !strcmp(argv[0], "--dostype")) {
	    if (argc-- > 1 && !DosType) {
		DosType = argv[1];
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
    if (!Uninstall && !RestoreBootBlock && !SaveBootBlock && !DoChecksum &&
	!DoCreateReset)
        Install = 1;
    else if (DoBackup && !DoCreateReset)
	Die("You can't specify --backup with --uninstall, --restore-from, "
	    "--save-to or\n--checksum\n");
    if (RestoreBootBlock && Uninstall)
	Die("You can't specify both --restore-from and --uninstall\n");

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
    BackupFile = CreateBackupFileName();

    ReadBootBlock(Device,0);
    printf("%s: ", Device);
    DosTypeID = BootBlock.ID;
    for (i = 0; DosTypeEntries[i].ID; i++)
	if (BootBlock.ID == DosTypeEntries[i].ID)
	    break;
    if (DosTypeEntries[i].ID)
	printf("%s", DosTypeEntries[i].Description);
    else {
	printf("Type 0x%08lx", BootBlock.ID);
	if (!Force) {
	    printf("\n");
	    Die("Use option --force to force continue\n");
	}
	DosTypeID = ID_BOOT_DISK;
    }
    printf("%s, Boot block checksum is 0x%08lx (%s)\n",
	   BootBlock.LiloID == LILO_ID ? " (created by Lilo)" : "",
	   CalcChecksum(),
	   CalcChecksum() ? "Incorrect" : "OK");

    if (SaveBootBlock) {
	WriteFile(SaveBootBlock, &BootBlock, sizeof(BootBlock));
	printf("Boot block saved to file `%s'\n", SaveBootBlock);
    }
    if (RestoreBootBlock) {
	ReadBootBlock(RestoreBootBlock,0);
	printf("Boot block read from file `%s'\n", RestoreBootBlock);
    }
    if (Uninstall) {
	ReadBootBlock(BackupFile,0);
	printf("Original boot block read from file `%s'\n", BackupFile);
    }
    if (DosType)
	SetDosType();
    if (Install) {
	BackupBootBlock();
	CreateMapFile();
	LoaderSize = ReadFile(LoaderTemplate, (void **)&LoaderData);
	if (!(MapVector = CreateVector(MapFile, &MapNumBlocks)))
	    Error_Open(MapFile);
	PatchLoader();
	WriteLoader();
	if (!(LoaderVector = CreateVector(LoaderFile, &LoaderNumBlocks)))
	    Error_Open(LoaderFile);
	CreateBootBlock(0);
    }
    if (DoCreateReset) {
	BackupBootBlock();
	CreateBootBlock(1);
    }
    if (DoChecksum)
	FixChecksum();
    if (RestoreBootBlock || Install || Uninstall || DoChecksum || DoCreateReset)
	WriteBootBlock(Device,0);
    exit(0);
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
