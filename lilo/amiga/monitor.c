/*
 *  Amiga Linux/m68k Loader -- Boot Monitor
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: monitor.c,v 1.3 1998-04-06 01:40:58 dorchain Exp $
 * 
 * $Log: monitor.c,v $
 * Revision 1.3  1998-04-06 01:40:58  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.2  1997/09/19 09:06:54  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:05  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <stddef.h>
#include <sys/types.h>
#include <asm/amigahw.h>

#include "strlib.h"

#include "bootstrap.h"
#include "linuxboot.h"
#include "lilo.h"
#include "config.h"
#include "loader.h"


static int ExitFlag;

struct Command {
    const char *Name;
    void (*Function)(int argc, const char *argv[]);
};

struct Value {
    const char *Name;
    long Value;
};

#define arraysize(x)	(sizeof(x)/sizeof(*(x)))


    /*
     *  Function Prototypes
     */

void Monitor(void);
static void CreateArgs(char *cmdline, int *argcp, const char **argvp[]);
static void DeleteArgs(int argc, const char *argv[]);
static int PartStrCaseCmp(const char *s1, const char *s2);
static int ExecCommand(const struct Command commands[], u_int numcommands,
		       int argc, const char *argv[]);
static int GetValue(const struct Value values[], u_int numvalues,
		    const char *name, long *value);
static void DumpInfo(const struct BootData *data);
static void ParseCommandLine(char *line);
static void Moni_help(int argc, const char *argv[]);
static void Moni_version(int argc, const char *argv[]);
static void Moni_list(int argc, const char *argv[]);
static void Moni_list_images(int argc, const char *argv[]);
static void Moni_list_files(int argc, const char *argv[]);
static void Moni_boot(int argc, const char *argv[]);
static void Moni_use(int argc, const char *argv[]);
static void Moni_info(int argc, const char *argv[]);
static void Moni_set(int argc, const char *argv[]);
static void Moni_set_type(int argc, const char *argv[]);
static void Moni_set_kernel(int argc, const char *argv[]);
static void Moni_set_ramdisk(int argc, const char *argv[]);
static void Moni_set_args(int argc, const char *argv[]);
static void Moni_set_debug(int argc, const char *argv[]);
static void Moni_set_model(int argc, const char *argv[]);
static void Moni_set_proc(int argc, const char *argv[]);
static void Moni_set_chip(int argc, const char *argv[]);
static void Moni_set_nummem(int argc, const char *argv[]);
static void Moni_set_mem(int argc, const char *argv[]);


    /*
     *  The Boot Monitor
     */

void Monitor(void)
{
    char *line;

    Puts("\nEntering Lilo Boot Monitor.\n");
    ExitFlag = 0;
    do {
	Puts("lilo> ");
	line = ReadCommandLine();
	ParseCommandLine(line);
    } while (!ExitFlag);
}


    /*
     *  Split a line into arguments
     */

static void CreateArgs(char *cmdline, int *argcp, const char **argvp[])
{
    int argc = 0, i;
    char *p, *q;
    const char **argv = NULL;

    *argcp = 0;
    p = cmdline;
    for (argc = 0;; argc++) {
	while (*p == ' ' || *p == '\t')
	    p++;
	if (!*p)
	    break;
	if (*p == '"') {
	    for (p++; *p && *p != '"'; p++)
		if (*p == '\\') {
		    p++;
		    if (!*p)
			break;
		}
	    if (*p)
		p++;
	} else
	    while (*p && *p != ' ' && *p != '\t')
		p++;
    }
    if (argc) {
	if (!(argv = Alloc((argc+1)*sizeof(char *))))
	    return;
	p = cmdline;
	for (i = 0; i < argc; i++) {
	    while (*p == ' ' || *p == '\t')
		p++;
	    if (!*p)
		break;
	    if (*p == '"') {
		q = p++;
		argv[i] = q;
		while (*p && *p != '"') {
		    if (*p == '\\') {
			p++;
			if (!*p)
			    break;
		    }
		    *q++ = *p++;
		}
	    } else {
		q = p;
		argv[i] = q;
		while (*p && *p != ' ' && *p != '\t') {
		    if (*p == '\\') {
			p++;
			if (!*p)
			    break;
		    }
		    *q++ = *p++;
		}
	    }
	    *q = '\0';
	    p++;
	}
    }
    *argcp = argc;
    *argvp = argv;
}


static void DeleteArgs(int argc, const char *argv[])
{
    if (argc)
	Free(argv);
}


    /*
     *  Partial strcasecmp() (s1 may be an abbreviation for s2)
     */

static int PartStrCaseCmp(const char *s1, const char *s2)
{
    char c1, c2;

    while (*s1) {
	c1 = *s1++;
	if (c1 >= 'A' && c1 <= 'Z')
	    c1 += ('a'-'A');
	c2 = *s2++;
	if (c2 >= 'A' && c2 <= 'Z')
	    c2 += ('a'-'A');
	if (!c2 || c1 != c2)
	    return(0);
    }
    return(1);
}


    /*
     *  Execute a Command
     */

static int ExecCommand(const struct Command commands[], u_int numcommands,
		       int argc, const char *argv[])
{
    u_int i;

    if (argc)
	for (i = 0; i < numcommands; i++)
	    if (PartStrCaseCmp(argv[0], commands[i].Name)) {
		commands[i].Function(argc-1, argv+1);
		return(1);
	    }
    return(0);
}


    /*
     *  Get a Value
     */

static int GetValue(const struct Value values[], u_int numvalues,
		    const char *name, long *value)
{
    u_int i;

    if (name)
	for (i = 0; i < numvalues; i++)
	    if (PartStrCaseCmp(name, values[i].Name)) {
		*value = values[i].Value;
		return(1);
	    }
    return(0);
}


static void DumpInfo(const struct BootData *data)
{
    int i;
    const char *p;

    PutChar('\n');
    switch (data->OSType) {
	case BOS_AMIGA:
	    p = "amiga (AmigaOS)";
	    break;
	case BOS_LINUX:
	    p = "linux (Linux/m68k)";
	    break;
	case BOS_NETBSD:
	    p = "netbsd (NetBSD-Amiga)";
	    break;
	default:
	    p = NULL;
	    break;
    }
    if (p)
	Printf("[type]    OS Type:          %s\n", p);
    else
	Printf("[type]    OS Type:          Unknown (%ld)\n", data->OSType);
    Printf("[kernel]  Kernel:           %s\n", data->Kernel);
    Printf("[ramdisk] Ramdisk:          %s\n", data->Ramdisk);
    Printf("[args]    Arguments:        %s\n", data->Args);
    Printf("[debug]   Debug Mode:       %s\n", data->Debug ? "TRUE" : "FALSE");
    if (data->Model == AMI_UNKNOWN)
	p = "auto";
    else if (data->Model >= first_amiga_model &&
    	     data->Model <= last_amiga_model)
        p = amiga_models[data->Model-first_amiga_model];
    if (p)
	Printf("[model]   Amiga Model:      %s\n", p);
    else
	Printf("[model]   Amiga Model:      Unknown (%ld)\n", data->Model);
    if (!data->Processor)
	Puts("[proc]    Processor type:   auto\n");
    else
	Printf("[proc]    Processor type:   %ld\n", data->Processor);
    if (!data->ChipSize)
	Puts("[chip]    Chip RAM Size:    auto\n");
    else
	Printf("[chip]    Chip RAM Size:    0x%08lx (%ld)\n", data->ChipSize,
	   data->ChipSize);
    if (!data->NumMemory)
	Puts("[nummem]  Memory Blocks:    auto\n");
    else {
	Printf("[nummem]  Memory Blocks:    %d\n", data->NumMemory);
	for (i = 0; i < data->NumMemory; i++)
	    Printf("[mem %2d]  0x%08lx: 0x%08lx bytes\n", i+1,
		   data->FastChunks[i].Address, data->FastChunks[i].Size);
    }
    PutChar('\n');
}


    /*
     *  Parse the Monitor Command Line
     */

static const struct Command Commands[] = {
    { "help", Moni_help },
    { "?", Moni_help },
    { "version", Moni_version },
    { "list", Moni_list },
    { "boot", Moni_boot },
    { "use", Moni_use },
    { "info", Moni_info },
    { "set", Moni_set }
};

static void ParseCommandLine(char *line)
{
    int argc;
    const char **argv;

    CreateArgs(line, &argc, &argv);
    if (argc && !ExecCommand(Commands, arraysize(Commands), argc, argv))
	Puts("Unknown command\n");
    DeleteArgs(argc, argv);
}


static void Moni_help(int argc, const char *argv[])
{
    Puts("\nLilo Boot Monitor\n\n"
	 "   help, ?                 Display this help\n"
	 "   version                 Version information\n"
	 "   list [images|files]     List all boot records and files\n"
	 "   boot [<label>]          Boot an Operating System\n"
	 "   use [<label>]           Use the specified boot record\n"
	 "   info [<label>]          Get information about a boot record\n"
	 "   set [<var>] [<value>]   Set boot variables\n\n");
}


static void Moni_version(int argc, const char *argv[])
{
    Printf("%s\n", LiloVersion);
}


static const struct Command Commands_list[] = {
    { "images", Moni_list_images },
    { "files", Moni_list_files }
};

static void Moni_list(int argc, const char *argv[])
{
    if (!argc) {
	ListRecords();
	ListFiles();
	PutChar('\n');
    } else if (!ExecCommand(Commands_list, arraysize(Commands_list), argc,
			    argv))
	Printf("Unknown list type\n");
}

static void Moni_list_images(int argc, const char *argv[])
{
    ListRecords();
    PutChar('\n');
}

static void Moni_list_files(int argc, const char *argv[])
{
    ListFiles();
    PutChar('\n');
}


static void Moni_boot(int argc, const char *argv[])
{
    const struct BootRecord *record;

    if (argc)
	if ((record = FindBootRecord(argv[0]))) {
	    FillBootData(record, &BootData, 0);
	    ExitFlag = 1;
	} else
	    Puts("Nonexistent boot image\n");
    else
	ExitFlag = 1;
}


static void Moni_use(int argc, const char *argv[])
{
    const struct BootRecord *record;
    const char *label = argc ? argv[0] : NULL;

    if ((record = FindBootRecord(label)))
	FillBootData(record, &BootData, 0);
    else
	Puts("Nonexistent boot image\n");
}


static void Moni_info(int argc, const char *argv[])
{
    const struct BootRecord *record;
    struct BootData data;

    if (!argc)
	DumpInfo(&BootData);
    else if ((record = FindBootRecord(argv[0]))) {
	FillBootData(record, &data, 0);
	DumpInfo(&data);
    } else
	Puts("Nonexistent boot image\n");
}


static const struct Command Commands_set[] = {
    { "type", Moni_set_type },
    { "kernel", Moni_set_kernel },
    { "ramdisk", Moni_set_ramdisk },
    { "args", Moni_set_args },
    { "debug", Moni_set_debug },
    { "model", Moni_set_model },
    { "proc", Moni_set_proc },
    { "chip", Moni_set_chip },
    { "nummem", Moni_set_nummem },
    { "mem", Moni_set_mem }
};

static void Moni_set(int argc, const char *argv[])
{
    if (!argc)
	DumpInfo(&BootData);
    else if (!ExecCommand(Commands_set, arraysize(Commands_set), argc, argv))
	Printf("Unknown variable\n");
}

static const struct Value Values_set_type[] = {
    { "amiga", BOS_AMIGA },
    { "linux", BOS_LINUX },
    { "netbsd", BOS_NETBSD }
};

static void Moni_set_type(int argc, const char *argv[])
{
    if (!argc)
	Puts("You should specify an Operating System type\n");
    else if (!GetValue(Values_set_type, arraysize(Values_set_type), argv[0],
		       &BootData.OSType))
	Puts("Unknown Operating System type\n");
}

static void Moni_set_kernel(int argc, const char *argv[])
{
    if (!argc)
	BootData.Kernel[0] = '\0';
    else {
	strncpy(BootData.Kernel, argv[0], PATH_SIZE);
	BootData.Kernel[PATH_SIZE-1] = '\0';
    }
}

static void Moni_set_ramdisk(int argc, const char *argv[])
{
    if (!argc)
	BootData.Ramdisk[0] = '\0';
    else {
	strncpy(BootData.Ramdisk, argv[0], PATH_SIZE);
	BootData.Ramdisk[PATH_SIZE-1] = '\0';
    }
}

static void Moni_set_args(int argc, const char *argv[])
{
    int i;

    BootData.Args[0] = '\0';
    for (i = 0; argc; argc--)
	if ((i+strlen(*argv)+1) < CL_SIZE) {
	    i += strlen(*argv)+1;
	    if (BootData.Args[0])
		strcat(BootData.Args, " ");
	    strcat(BootData.Args, *argv++);
	}
}

static const struct Value Values_set_debug[] = {
    { "true", 1 },
    { "1", 1 },
    { "false", 0 },
    { "0", 0 }
};

static void Moni_set_debug(int argc, const char *argv[])
{
    if (!argc)
	BootData.Debug = 1;
    else if (!GetValue(Values_set_debug, arraysize(Values_set_debug), argv[0],
		       &BootData.Debug))
	Printf("Unknown boolean value\n");
}

static const struct Value Values_set_model[] = {
    { "auto", AMI_UNKNOWN },
    { "A500", AMI_500 },
    { "A500+", AMI_500PLUS },
    { "A600", AMI_600 },
    { "A1000", AMI_1000 },
    { "A1200", AMI_1200 },
    { "A2000", AMI_2000 },
    { "A2500", AMI_2500 },
    { "A3000", AMI_3000 },
    { "A3000T", AMI_3000T },
    { "A3000+", AMI_3000PLUS },
    { "A4000", AMI_4000 },
    { "A4000T", AMI_4000T },
    { "CDTV", AMI_CDTV },
    { "CD32", AMI_CD32 },
    { "Draco", AMI_DRACO }
};

static void Moni_set_model(int argc, const char *argv[])
{
    if (!argc)
	BootData.Model = AMI_UNKNOWN;
    else if (GetValue(Values_set_model, arraysize(Values_set_model), argv[0],
		      &BootData.Model))
	BootData.Model = strtoul(argv[0], NULL, 0);
}

static void Moni_set_proc(int argc, const char *argv[])
{
    if (!argc || PartStrCaseCmp(argv[0], "auto"))
	BootData.Processor = 0;
    else
	BootData.Processor = strtoul(argv[0], NULL, 0);
}

static void Moni_set_chip(int argc, const char *argv[])
{
    if (!argc)
	Puts("You should specify the Chip RAM size\n");
    else if (PartStrCaseCmp(argv[0], "auto"))
	BootData.ChipSize = 0;
    else
	BootData.ChipSize = strtoul(argv[0], NULL, 0);
}

static void Moni_set_nummem(int argc, const char *argv[])
{
    u_long i;

    if (!argc || PartStrCaseCmp(argv[0], "auto"))
	BootData.NumMemory = 0;
    else {
	i = strtoul(argv[0], NULL, 0);
	if (!i)
	    BootData.NumMemory = 0;
	else if (i > NUM_MEMINFO)
	    Puts("Invalid number of memory blocks\n");
	else
	    BootData.NumMemory = i;
    }
}

static void Moni_set_mem(int argc, const char *argv[])
{
    u_long i;

    if (argc < 3)
	Puts("You should specify a memory block number, address and size\n");
    else {
	i = strtoul(argv[0], NULL, 0);
	if (i < 1 || i > BootData.NumMemory)
	    Puts("Invalid memory block number\n");
	else {
	    BootData.FastChunks[i-1].Address = strtoul(argv[1], NULL, 0);
	    BootData.FastChunks[i-1].Size = strtoul(argv[2], NULL, 0);
	}
    }
}
