/*
 *  Amiga Linux/m68k Loader
 *
 *  Dump the contents of a map file (for debugging)
 *
 *  © Copyright 1996 by Geert Uytterhoeven
 *		       (Geert.Uytterhoeven@cs.kuleuven.ac.be)
 *
 *  --------------------------------------------------------------------------
 *
 *  Usage:
 *
 *      fakeboot [-v|--verbose] [<mapfile>]
 *
 *  With:
 *
 *      <mapfile>   : path of the map file (default: `/boot/map')
 *
 *  --------------------------------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 */


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

#include "../lilo.h"


static const char *ProgramName;


void __attribute__ ((noreturn, format (printf, 1, 2))) Die(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}


void __attribute__ ((noreturn)) Usage(void)
{
    Die("Usage: %s [-v|--verbose] [mapfile]\n", ProgramName);
}


int main(int argc, char *argv[])
{
    int fh, size = 0;
    u_char *data;
    const struct TagRecord *tr;
    u_long offset = 0;
    const char *mapfile = NULL;
    int verbose = 0;

    ProgramName = argv[0];
    argc--;
    argv++;

    while (argc) {
	if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
	    Usage();
	else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose"))
	    verbose = 1;
	else if (!mapfile)
	    mapfile = argv[0];
	else
	    Usage();
	argc--;
	argv++;
    }
    if (!mapfile)
	mapfile = LILO_MAPFILE;

    if ((fh = open(mapfile, O_RDONLY)) == -1)
	Die("open %s: %s\n", mapfile, strerror(errno));
    if ((size = lseek(fh, 0, SEEK_END)) == -1)
	Die("lseek %s: %s\n", mapfile, strerror(errno));
    if (!(data = malloc(size)))
	Die("No memory\n");
    if (lseek(fh, 0, SEEK_SET) == -1)
	Die("lseek %s: %s\n", mapfile, strerror(errno));
    if (read(fh, data, size) != size)
	Die("read %s: %s\n", mapfile, strerror(errno));
    close(fh);

    while (1) {
	tr = (struct TagRecord *)&data[offset];
	if (verbose)
	    printf("0x%08lx: tag = 0x%08lx, size = 0x%08lx\n\t", offset,
		   tr->Tag, tr->Size);
	switch (tr->Tag) {
	    case TAG_LILO:
		printf("  TAG_LILO: File Identification\n");
		break;
	    case TAG_EOF:
		printf("  TAG_EOF: End of File\n");
		break;
	    case TAG_HEADER:
		printf("    TAG_HEADER: Start of Boot Options\n");
		break;
	    case TAG_HEADER_END:
		printf("    TAG_HEADER_END: End of Boot Options\n");
		break;
	    case TAG_DEFAULT:
		printf("      TAG_DEFAULT: Default Boot Record = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_AUTO:
		printf("      TAG_AUTO: Auto = %ld\n", tr->Data[0]);
		break;
	    case TAG_TIME_OUT:
		printf("      TAG_TIME_OUT: Time Out = %ld seconds\n",
		       tr->Data[0]);
		break;
	    case TAG_AUX:
		printf("      TAG_AUX: Aux = %ld\n", tr->Data[0]);
		break;
	    case TAG_BAUD:
		printf("      TAG_BAUD: Baud = %ld\n", tr->Data[0]);
		break;
	    case TAG_MASTER_PASSWORD:
		printf("      TAG_MASTER_PASSWORD: Master Password = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_DEBUG:
		printf("      TAG_DEBUG: Debug = %ld\n", tr->Data[0]);
		break;
	    case TAG_PROMPT:
		printf("      TAG_PROMPT: Boot Prompt = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_KICKRANGE:
		printf("      TAG_KICKRANGE: Valid Kickstart Range = %d-%d\n",
		       ((u_short *)tr->Data)[0], ((u_short *)tr->Data)[1]);
		break;
	    case TAG_G_CHIP_RAM_SIZE:
		printf("      TAG_G_CHIP_RAM_SIZE: 0x%08lx bytes of Chip RAM\n",
		       tr->Data[0]);
		break;
	    case TAG_G_FAST_RAM_CHUNK:
		printf("      TAG_G_FAST_RAM_CHUNK: 0x%08lx bytes at 0x%08lx\n",
		       tr->Data[1], tr->Data[0]);
		break;
	    case TAG_G_MODEL:
		printf("      TAG_G_MODEL: Model = %ld\n", tr->Data[0]);
		break;
	    case TAG_G_PROCESSOR:
		printf("      TAG_G_PROCESSOR: Processor = %ld\n", tr->Data[0]);
		break;
	    case TAG_BOOT_RECORD:
		printf("    TAG_BOOT_RECORD: Start of Boot Record `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_BOOT_RECORD_END:
		printf("    TAG_BOOT_RECORD_END: End of Boot Record\n");
		break;
	    case TAG_ALIAS:
		printf("      TAG_ALIAS: Alias Label = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_OS_TYPE:
		printf("      TAG_OS_TYPE: Operating System type = %ld\n",
		       tr->Data[0]);
		break;
	    case TAG_KERNEL:
		printf("      TAG_KERNEL: Kernel Image = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_ARGUMENTS:
		printf("      TAG_ARGUMENTS: Boot Arguments = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_PASSWORD:
		printf("      TAG_PASSWORD: Boot Record Password = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_CHIP_RAM_SIZE:
		printf("      TAG_CHIP_RAM_SIZE: 0x%08lx bytes of Chip RAM\n",
		       tr->Data[0]);
		break;
	    case TAG_FAST_RAM_CHUNK:
		printf("      TAG_FAST_RAM_CHUNK: 0x%08lx bytes at 0x%08lx\n",
		       tr->Data[1], tr->Data[0]);
		break;
	    case TAG_MODEL:
		printf("      TAG_MODEL: Model = %ld\n", tr->Data[0]);
		break;
	    case TAG_RAMDISK:
		printf("      TAG_RAMDISK: Ramdisk Image = `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_PROCESSOR:
		printf("      TAG_PROCESSOR: Processor = %ld\n", tr->Data[0]);
		break;
	    case TAG_FILE_DEF:
		printf("    TAG_FILE_DEF: Start of File Definition `%s'\n",
		       (char *)tr->Data);
		break;
	    case TAG_FILE_DEF_END:
		printf("    TAG_FILE_DEF_END: End of File Definition\n");
		break;
	    case TAG_VECTOR:
		printf("      TAG_VECTOR: Vector containing %ld bytes\n",
		       tr->Data[0]);
		break;
	    default:
		printf("    <UNKNOWN>\n");
		break;
	}
	offset += sizeof(struct TagRecord)+tr->Size;
	if (offset > size) {
	    puts("Warning: Beyond end of file");
	    break;
	}
	if (tr->Tag == TAG_EOF)
	    break;
    }
    if (offset < size)
	printf("Warning: Left %ld bytes\n", size-offset);
    return(0);
}
