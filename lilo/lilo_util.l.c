/*
 *  Linux/m68k Loader -- General Utilities
 *
 *  � Copyright 1995-97 by Geert Uytterhoeven, Roman Hodek
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
 * $Id: lilo_util.l.c,v 1.5 1998-02-26 10:07:56 rnhodek Exp $
 * 
 * $Log: lilo_util.l.c,v $
 * Revision 1.5  1998-02-26 10:07:56  rnhodek
 * CreateFileName: Don't prefix filename with 'Root' if it has a "bootp:" prefix.
 *
 * Revision 1.4  1998/02/19 20:40:13  rnhodek
 * Make things compile again
 *
 * Revision 1.3  1997/08/23 23:10:41  rnhodek
 * Add sector number in printout of {Read,Write}BootBlock
 * Fix bug in map calculation
 *
 * Revision 1.2  1997/08/12 21:51:02  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:26:57  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/hdreg.h>
#define _LINUX_STAT_H		/* Hack to prevent including <linux/stat.h> */
struct inode;			/* to avoid warning */
#include <linux/fs.h>

#include "lilo.h"
#include "lilo_util.h"
#include "config.h"
#include "parser.h"
#include "minmax.h"

    /*
     *	Global Common Variables
     */

const char LiloVersion[] = LILO_VERSION;

const char *ProgramName = NULL;
int Verbose = 0;
int DoBackup = 0;
int Install = 0;
int Uninstall = 0;

struct BootBlock BootBlock;
const char *SaveBootBlock = NULL;
const char *RestoreBootBlock = NULL;
const char *Device = NULL;
const char *Root = NULL;
const char *ConfigFile = NULL;
const char *MapFile;
const char *LoaderTemplate;
const char *LoaderFile;
const char *BackupFile;
dev_t BootDevice;
u_long HoleSector;
u_long MaxHoleSectors;
const struct vecent *MapVector;
int MapNumBlocks;
char *LoaderData;
int LoaderSize;
const struct vecent *LoaderVector;
int LoaderNumBlocks;


    /*
     *	Print an Error Message and Exit
     */

void Die(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}


    /*
     *  Error Conditions
     */

void Error_NoMemory(void)
{
    Die("Can't allocate memory\n");
}

void Error_Open(const char *name)
{
    Die("Can't open file `%s': %s\n", name, strerror(errno));
}

void Error_OpenDir(const char *name)
{
    Die("Can't open directory `%s': %s\n", name, strerror(errno));
}

void Error_Read(const char *name)
{
    Die("Can't read from file `%s': %s\n", name, strerror(errno));
}

void Error_Write(const char *name)
{
    Die("Can't write to file `%s': %s\n", name, strerror(errno));
}

void Error_Seek(const char *name)
{
    Die("Can't seek in file `%s': %s\n", name, strerror(errno));
}

void Error_Stat(const char *name)
{
    Die("Can't get info about file `%s': %s\n", name, strerror(errno));
}

void Error_Ioctl(const char *name, const char *ioctl)
{
    Die("Operation `%s' failed for file `%s': %s\n", ioctl, name,
	strerror(errno));
}


    /*
     *	Read a File
     */

int ReadFile(const char *name, void **data)
{
    int fh, size = 0;

    if (Verbose)
	printf("Reading file `%s'\n", name);

    if ((fh = open(name, O_RDONLY)) == -1)
	Error_Open(name);
    if ((size = lseek(fh, 0, SEEK_END)) == -1)
	Error_Seek(name);
    if (!(*data = malloc(size)))
	Error_NoMemory();
    if (lseek(fh, 0, SEEK_SET) == -1)
	Error_Seek(name);
    if (read(fh, *data, size) != size)
	Error_Read(name);
    close(fh);
    return(size);
}


    /*
     *	Write a File
     */

void WriteFile(const char *name, const void *data, int size)
{
    int fh;

    if (Verbose)
	printf("Writing file `%s'\n", name);

    if ((fh = open(name, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
	Error_Open(name);
    if (write(fh, data, size) != size)
	Error_Write(name);
    close(fh);
}


    /*
     *	Read a Boot Block from a Device or File
     */

void ReadBootBlock(const char *name, u_long sector)
{
    int fh;

    if (Verbose)
	printf("Reading boot block from `%s' (sector %lu)\n", name, sector);

    if ((fh = open(name, O_RDONLY)) == -1)
	Error_Open(name);
    if (lseek(fh, sector*HARD_SECTOR_SIZE, SEEK_SET) < 0)
	Error_Seek(name);
    if (read(fh, &BootBlock, sizeof(BootBlock)) != sizeof(BootBlock))
	Error_Read(name);
    close(fh);
}


    /*
     *	Write a Boot Block to a Device
     */

void WriteBootBlock(const char *name, u_long sector)
{
    int fh;

    if (Verbose)
	printf("Writing boot block to device `%s' (sector %lu)\n",
	       name, sector);

    if ((fh = open(name, O_RDWR)) == -1)
	Error_Open(name);
    if (lseek(fh, sector*HARD_SECTOR_SIZE, SEEK_SET) < 0)
	Error_Seek(name);
    if (write(fh, &BootBlock, sizeof(BootBlock)) != sizeof(BootBlock))
	Error_Write(name);
    close(fh);
}


    /*
     *  Backup the Boot Block if it wasn't backed up before
     */

void BackupBootBlock(void)
{
    struct stat info;

    if (stat(BackupFile, &info) == -1 || DoBackup) {
	WriteFile(BackupFile, &BootBlock, sizeof(BootBlock));
	printf("Boot block backed up to file `%s'\n", BackupFile);
    } else if (Verbose)
        printf("Boot block backup `%s' already exists. No backup taken.\n",
	       BackupFile);
}


    /*
     *	Read the Configuration File
     */

void ReadConfigFile(void)
{
    if (Verbose)
	printf("Reading configuration file `%s'\n", ConfigFile);

    if (!(confin = fopen(ConfigFile, "r")))
	Error_Open(ConfigFile);
    confparse();
    fclose(confin);

    Device = Config.BootDevice;
    if (!Config.Options.Default)
	Config.Options.Default = Config.Records->Label;
    if (Config.Options.Prompt)
	FixSpecialChars((char *)Config.Options.Prompt);
}

    /*
     *	Check whether two strings exist and are equal
     */

int EqualStrings(const char *s1, const char *s2)
{
    return(s1 && s2 && !strcmp(s1, s2) ? 1 : 0);
}


    /*
     *  Interprete the special characters in a string
     */

void FixSpecialChars(char *s)
{
    char *d = s;

    while (*s)
	if (s[0] == '\\' && s[1])
	    switch (s[1]) {
		case 'b':
		    *d++ = '\b';
		    s += 2;
		    break;
		case 'e':
		    *d++ = '\e';
		    s += 2;
		    break;
		case 'n':
		    *d++ = '\n';
		    s += 2;
		    break;
		case 'r':
		    *d++ = '\r';
		    s += 2;
		    break;
		case 't':
		    *d++ = '\t';
		    s += 2;
		    break;
		case '\\':
		    *d++ = '\\';
		    s += 2;
		    break;
		default:
		    *d++ = *s++;
		    break;
	    }
	else
	    *d++ = *s++;
    *d = '\0';
}


    /*
     *	Add a New Boot Record
     */

int AddBootRecord(const struct BootRecord *record)
{
    struct BootRecord **p, *new;

    for (p = &Config.Records; *p; p = &(*p)->Next)
	if (EqualStrings((*p)->Label, record->Label) ||
	    EqualStrings((*p)->Alias, record->Label) ||
	    EqualStrings((*p)->Label, record->Alias) ||
	    EqualStrings((*p)->Alias, record->Alias))
	    return(0);
    if (!(new = malloc(sizeof(struct BootRecord))))
	Error_NoMemory();
    *new = *record;
    *p = new;
    return(1);
}


    /*
     *	Add a New File Definition
     */

void AddFileDef(const char *path)
{
    struct FileDef **p, *new;

    for (p = &Config.Files; *p; p = &(*p)->Next)
	if (EqualStrings((*p)->Path, path))
	    return;
    if (!(new = malloc(sizeof(struct FileDef))))
	Error_NoMemory();
    new->Next = NULL;
    new->Path = path;
    new->Vector = NULL;
    *p = new;
}

    /*
     *	Create a Complete File Name
     */

const char *CreateFileName(const char *name)
{
    int size;
    char *s;
    int prefix_root;

#ifdef USE_BOOTP
    prefix_root = Root && (strncmp( name, "bootp:", 6 ) != 0);
#else
    prefix_root = Root != 0;
#endif
    
    size = strlen(name)+1;
    if (prefix_root)
	size += strlen(Root);
    if (!(s = malloc(size)))
	Error_NoMemory();
    if (prefix_root) {
	strcpy(s, Root);
	strcat(s, name);
    } else
	strcpy(s, name);
    return(s);
}


    /*
     *  Create the File Name for the Boot Block Backup
     */

const char *CreateBackupFileName(void)
{
    int size;
    char *s;

    size = strlen(LILO_BACKUPFILEPREFIX)+7;
    if (Root)
	size += strlen(Root);
    if (!(s = malloc(size)))
	Error_NoMemory();
    sprintf(s, "%s%s.%02x.%02x", Root ? Root : "", LILO_BACKUPFILEPREFIX,
	    MAJOR(BootDevice), MINOR(BootDevice));
    return(s);
}

    /*
     *	Create a Vector for a File
     */

const struct vecent *CreateVector( const char *name, int *numblocks )
{
    int fh, size, i, j, n, maxsize;
    struct vecent *vector = NULL;
    u_long blk, nblocks, offset, start, blksize, thisstart;
    dev_t device;
    int sectors_per_block;

    if (!name || ((fh = open(name, O_RDONLY)) == -1))
	return(NULL);
    if ((size = lseek(fh, 0, SEEK_END)) == -1)
	Error_Seek(name);
    maxsize = (size+HARD_SECTOR_SIZE-1)/HARD_SECTOR_SIZE;
    if (!(vector = malloc((maxsize+1)*sizeof(struct vecent))))
	Error_NoMemory();
    vector[0].start = size;

    GeometryFile(name, &device, &start);
    CheckVectorDevice( name, device, vector );

    if (ioctl(fh, FIGETBSZ, &blksize) == -1)
	Error_Ioctl(name, "FIGETBSZ");
    if (blksize % HARD_SECTOR_SIZE)
	Die("File `%s' resides on a device with an unsupported block size "
	    "(%ld bytes)\n", name, blksize);
    sectors_per_block = blksize/HARD_SECTOR_SIZE;
    nblocks = (size+blksize-1)/blksize;

    for( blk = 0, i = 0; blk < nblocks; ++blk ) {
	offset = blk;
	if (ioctl(fh, FIBMAP, &offset) == -1)
	    Error_Ioctl(name, "FIBMAP");

	if (offset == 0) {
	    /* hole in file */
	    for( j = sectors_per_block; j > 0; j -= n ) {
		if (i == 0 ||
		    vector[i].start != HoleSector ||
		    vector[i].length >= MaxHoleSectors) {
		    ++i;
		    vector[i].start  = HoleSector;
		    vector[i].length = 0;
		}
		n = min( MaxHoleSectors-vector[i].length, j );
		vector[i].length += n;
	    }
	}
	else {
	    /* not a hole */
	    thisstart = start + offset*sectors_per_block;
	    if (i != 0 && vector[i].start + vector[i].length == thisstart)
		vector[i].length += sectors_per_block;
	    else {
		++i;
		vector[i].start  = thisstart;
		vector[i].length = sectors_per_block;
	    }
	}
    }
    *numblocks = i;
    /* shrink vector's memory to what is really needed */
    if (!(vector = realloc( vector, (i+1)*sizeof(struct vecent) )))
	Error_NoMemory();

    close(fh);
    return(vector);
}


    /*
     *	Find the Name to a Device Number
     */

void FindDevice( dev_t device, char *devname )
{
    struct stat info;
    DIR *dir;
    const struct dirent *dirent;
    int found = 0;

    if (!(dir = opendir("/dev")))
	Error_OpenDir("/dev");
    while ((dirent = readdir(dir))) {
	sprintf(devname, "/dev/%s", dirent->d_name);
	if ((stat(devname, &info) != -1) && S_ISBLK(info.st_mode) &&
	    (info.st_rdev == device)) {
	    found = 1;
	    break;
	}
    }
    closedir(dir);
    if (!found)
	Die("Can't find special device entry for device %04lx\n",
	    (u_long)device);
}


    /*
     *	Get Name and Device Number of the Device on Which File 'name' Resides
     */

void FindFileDevice( const char *name, char *devname, dev_t *device )
{
    struct stat info;

    if (stat(name, &info) == -1)
	Error_Stat(name);
    if (!S_ISREG(info.st_mode))
	Die("%s is not a regular file\n", name);
    *device = info.st_dev;
    FindDevice( *device, devname );
}


    /*
     *	Get the Start Sector for the Partition a File belongs to
     */

void GeometryFile(const char *name, dev_t *device, u_long *start)
{
    char devname[PATH_MAX+1];

    FindFileDevice( name, devname, device );
    GeometryDevice( devname, start );
}

    /*
     *	Get the Start Sector of a Partition
     */

void GeometryDevice( const char *devname, u_long *start)
{
    int fh;
    struct hd_geometry geo;
    
    if ((fh = open(devname, O_RDONLY)) == -1)
	Error_Open(devname);
    if (ioctl(fh, HDIO_GETGEO, &geo) == -1)
	Error_Ioctl(devname, "HDIO_GETGEO");
    close(fh);
    *start = geo.start;
}


    /*
     *	Patch the Loader with the Map Vector
     */

void PatchLoader(void)
{
    int i;
    const u_long pattern[] = {
	LILO_ID, LILO_MAPVECTORID
    };
    u_long *data = NULL;
    u_long maxsize;

    for (i = 0; i < LoaderSize-sizeof(pattern)+1; i++)
	if (!memcmp(&LoaderData[i], pattern, sizeof(pattern))) {
	    data = (u_long *)&LoaderData[i];
	    break;
	}
    if (!data)
	Die("Couldn't find magic key in loader template\n");
    maxsize = data[2];
    if (MapNumBlocks > maxsize-1)
	Die("Map vector is too large for loader (%ld entries too much)\n",
	    MapNumBlocks-maxsize-1);
    if (Verbose)
	printf("%d entries for map vector blocks, %lu entries free\n",
	       MapNumBlocks, maxsize-1-MapNumBlocks);
    memcpy(&data[3], MapVector, (MapNumBlocks+1)*sizeof(struct vecent));
}


	
/* Local Variables: */
/* tab-width: 8     */
/* End:             */
