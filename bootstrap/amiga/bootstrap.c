/*
** linux/arch/m68k/boot/amiga/bootstrap.c -- This program loads the Linux/m68k
**					     kernel into an Amiga and launches
**					     it.
**
** Copyright 1993,1994 by Hamish Macdonald, Greg Harp
**
** Modified 11-May-94 by Geert Uytterhoeven
**			(Geert.Uytterhoeven@cs.kuleuven.ac.be)
**     - A3640 MapROM check
** Modified 31-May-94 by Geert Uytterhoeven
**     - Memory thrash problem solved
** Modified 07-March-95 by Geert Uytterhoeven
**     - Memory block sizes are rounded to a multiple of 256K instead of 1M
**	 This _requires_ >0.9pl5 to work!
**	 (unless all block sizes are multiples of 1M :-)
** Modified 11-July-95 by Andreas Schwab
**     - Support for ELF kernel (untested!)
** Modified 10-Jan-96 by Geert Uytterhoeven
**     - The real Linux/m68k boot code moved to linuxboot.[ch]
** Modified 9-Sep-96 by Geert Uytterhoeven
**     - Rewritten option parsing
**     - New parameter passing to linuxboot() (linuxboot_args)
** Modified 6-Oct-96 by Geert Uytterhoeven
**     - Updated for the new boot information structure
**
** This file is subject to the terms and conditions of the GNU General Public
** License.  See the file COPYING in the main directory of this archive
** for more details.
**
** $Id: bootstrap.c,v 1.4 1997-09-19 09:06:22 geert Exp $
** 
** $Log: bootstrap.c,v $
** Revision 1.4  1997-09-19 09:06:22  geert
** Big bunch of changes by Geert: make things work on Amiga; cosmetic things
**
** Revision 1.3  1997/08/10 19:22:53  rnhodek
** Moved AmigaOS inline funcs to extr header inline-funcs.h; the functions
** can't be compiled under Linux
**
** Revision 1.2  1997/07/16 14:05:00  rnhodek
** Sorted out which headers to use and the like; Amiga bootstrap now compiles.
** Puts and other generic functions now defined in bootstrap.h
**
** Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
** Import sources into CVS
**
** 
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

/* required Linux/m68k include files */
#include <asm/amigahw.h>

/* Amiga bootstrap include files */
#include "linuxboot.h"
#include "inline-funcs.h"
#include "bootstrap.h"


/* Library Bases */
long __oslibversion = 36;
extern const struct ExecBase *SysBase;

static const char *memfile_name = NULL;

static int model = AMI_UNKNOWN;

static const char *ProgramName;

struct linuxboot_args args;


    /*
     *  Function Prototypes
     */

static void Usage(void) __attribute__ ((noreturn));
int main(int argc, char *argv[]);
static void Sleep(u_long micros);


static void Usage(void)
{
    fprintf(stderr,
	"Linux/m68k Amiga Bootstrap version " AMIBOOT_VERSION "\n\n"
	"Usage: %s [options] [kernel command line]\n\n"
	"Basic options:\n"
	"    -h, --help           Display this usage information\n"
	"    -k, --kernel file    Use kernel image `file' (default is `vmlinux')\n"
	"    -r, --ramdisk file   Use ramdisk image `file'\n"
	"Advanced options:\n"
	"    -d, --debug          Enable debug mode\n"
	"    -b, --baud speed     Set the serial port speed (default is 9600)\n"
	"    -m, --memfile file   Use memory file `file'\n"
	"    -v, --keep-video     Don't reset the video mode\n"
	"    -t, --model id       Set the Amiga model to `id'\n"
	"    -p, --processor cfm  Set the processor type to `cfm\n\n",
	ProgramName);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    int i;
    int processor = 0, debugflag = 0, keep_video = 0;
    u_int baud = 0;
    const char *kernel_name = NULL;
    const char *ramdisk_name = NULL;
    char commandline[CL_SIZE] = "";

    ProgramName = argv[0];
    while (--argc) {
	argv++;
	if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help"))
	    Usage();
	else if (!strcmp(argv[0], "-k") || !strcmp(argv[0], "--kernel"))
	    if (--argc && !kernel_name) {
		kernel_name = argv[1];
		argv++;
	    } else
		Usage();
	else if (!strcmp(argv[0], "-r") || !strcmp(argv[0], "--ramdisk"))
	    if (--argc && !ramdisk_name) {
		ramdisk_name = argv[1];
		argv++;
	    } else
		Usage();
	else if (!strcmp(argv[0], "-d") || !strcmp(argv[0], "--debug"))
	    debugflag = 1;
	else if (!strcmp(argv[0], "-b") || !strcmp(argv[0], "--baud"))
	    if (--argc && !baud) {
		baud = atoi(argv[1]);
		argv++;
	    } else
		Usage();
	else if (!strcmp(argv[0], "-m") || !strcmp(argv[0], "--memfile"))
	    if (--argc && !memfile_name) {
		memfile_name = argv[1];
		argv++;
	    } else
		Usage();
	else if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--keep-video"))
	    keep_video = 1;
	else if (!strcmp(argv[0], "-t") || !strcmp(argv[0], "--model"))
	    if (--argc && !model) {
		model = atoi(argv[1]);
		argv++;
	    } else
		Usage();
	else if (!strcmp(argv[0], "-p") || !strcmp(argv[0], "--processor"))
	    if (--argc && !processor) {
		processor = atoi(argv[1]);
		argv++;
	    } else
		Usage();
	else
	    break;
    }
    if (!kernel_name)
	kernel_name = "vmlinux";

    SysBase = *(struct ExecBase **)4;

    /*
     *	Join command line options
     */
    i = 0;
    while (argc--) {
	if ((i+strlen(*argv)+1) < CL_SIZE) {
	    i += strlen(*argv) + 1;
	    if (commandline[0])
		strcat(commandline, " ");
	    strcat(commandline, *argv++);
	}
    }

    memset(&args.bi, 0, sizeof(args.bi));
    if (processor) {
	int cpu = processor/100%10;
	int fpu = processor/10%10;
	int mmu = processor%10;
	if (cpu)
	    args.bi.cputype = 1<<(cpu-1);
	if (fpu)
	    args.bi.fputype = 1<<(fpu-1);
	if (mmu)
	    args.bi.mmutype = 1<<(mmu-1);
    }
    /*
     *	If we have a memory file, read the memory information from it
     */
    if (memfile_name) {
	FILE *fp;
	int i;

	if ((fp = fopen(memfile_name, "r")) == NULL) {
	    perror("open memory file");
	    fprintf(stderr, "Cannot open memory file %s\n", memfile_name);
	    return(FALSE);
	}

	if (fscanf(fp, "%lu", &args.bi.chip_size) != 1) {
	    fprintf(stderr, "memory file does not contain chip memory size\n");
	    fclose(fp);
	    return(FALSE);
	}

	for (i = 0; i < NUM_MEMINFO; i++)
	    if (fscanf(fp, "%lx %lu", &args.bi.memory[i].addr,
		       &args.bi.memory[i].size) != 2)
		break;

	fclose(fp);
	args.bi.num_memory = i;
    }
    strncpy(args.bi.command_line, commandline, CL_SIZE);
    args.bi.command_line[CL_SIZE-1] = '\0';
    if (model != AMI_UNKNOWN)
	args.bi.model = model;

    args.kernelname = kernel_name;
    args.ramdiskname = ramdisk_name;
    args.debugflag = debugflag;
    args.keep_video = keep_video;
    args.reset_boards = 1;
    args.baud = baud;
    args.sleep = Sleep;

    /* Do The Right Stuff */
    linuxboot(&args);

    /* if we ever get here, something went wrong */
    exit(EXIT_FAILURE);
}


    /*
     *  Routines needed by linuxboot
     */

static void Sleep(u_long micros)
{
    struct MsgPort *TimerPort;
    struct timerequest *TimerRequest;

    if ((TimerPort = CreateMsgPort())) {
	if ((TimerRequest = CreateIORequest(TimerPort,
					    sizeof(struct timerequest)))) {
	    if (!OpenDevice("timer.device", UNIT_VBLANK,
			    (struct IORequest *)TimerRequest, 0)) {
		TimerRequest->io_Command = TR_ADDREQUEST;
		TimerRequest->io_Flags = IOF_QUICK;
		TimerRequest->tv_secs = micros/1000000;
		TimerRequest->tv_micro = micros%1000000;
		DoIO((struct IORequest *)TimerRequest);
		CloseDevice((struct IORequest *)TimerRequest);
	    }
	    DeleteIORequest(TimerRequest);
	}
	DeleteMsgPort(TimerPort);
    }
}
