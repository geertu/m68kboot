/*
 *  Amiga Linux/m68k Loader -- Loader and User Interface
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: loader.c,v 1.6 2004-10-01 19:01:33 geert Exp $
 * 
 * $Log: loader.c,v $
 * Revision 1.6  2004-10-01 19:01:33  geert
 * Kill warning about unused variable saved_return_address by moving it to the
 * inline assembler.
 *
 * Revision 1.5  1998/04/06 01:40:57  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.4  1998/03/17 12:31:15  rnhodek
 * Change Puts with arg to Printf.
 *
 * Revision 1.3  1998/03/10 10:23:11  rnhodek
 * New option "message": print before showing the prompt.
 *
 * Revision 1.2  1997/09/19 09:06:54  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:04  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include "strlib.h"

#include <asm/amigahw.h>

#include "amigaos.h"
#include "config.h"
#include "lilo.h"
#include "console.h"
#include "loader.h"
#include "parsetags.h"
#include "lilo_util.h"


    /*
     *  Program Entry
     *
     *  Pass the Pointer to the IOStdReq for the Boot Device (in d0) to Main
     *  and fix offset table
     */

asm(".text
	.align 4\n"
".globl " SYMBOL_NAME_STR(_start) ";\n"
SYMBOL_NAME_STR(_start) ":
	movel	%d0,%sp@-	
	lea	%pc@("SYMBOL_NAME_STR(_start)"),%a0
	movel	%a0,%d0
	lea	%pc@(_GLOBAL_OFFSET_TABLE_),%a0
	lea	%pc@("SYMBOL_NAME_STR(_edata)"),%a1
1:	addl	%d0,%a0@+
	cmpl	%a1,%a0
	bcs	1b
	jbsr	%pc@(" SYMBOL_NAME_STR(Main) ")
	addql	#4,%sp
	rts
.globl " SYMBOL_NAME_STR(ptr_fkt_offset_jmp) ";\n"
SYMBOL_NAME_STR(ptr_fkt_offset_jmp) ":
.globl " SYMBOL_NAME_STR(int_fkt_offset_jmp) ";\n"
SYMBOL_NAME_STR(int_fkt_offset_jmp) ":
	lea	%pc@("SYMBOL_NAME_STR(saved_return_address)"),%a0
	movel	%sp@+,%a0@
	movel	%sp@+,%d0
	lea	%pc@("SYMBOL_NAME_STR(_start)"),%a0
	addl	%d0,%a0
	jsr	%a0@
	subql	#4,%sp
	lea     %pc@("SYMBOL_NAME_STR(saved_return_address)"),%a1
	movel	%a1@,%a1
	jmp	%a1@	|return
" SYMBOL_NAME_STR(saved_return_address) ":
	.long	0
");
/* gcc returns pointers in %a0, data values in %d0, so... */
void *ptr_fkt_offset_jmp(void *,...);
u_long int_fkt_offset_jmp(void *,...);

const char LiloVersion[] = VERSION;

int Debug = 0;


    /*
     *	Console Echo Modes
     */

#define ECHO_NONE	(0)
#define ECHO_NORMAL	(1)
#define ECHO_DOT	(2)


    /*
     *	Boot Library Functions
     */

static struct IOStdReq *BootRequest;


    /*
     *	Library Bases
     */

const struct ExecBase *SysBase = NULL;
const struct ExpansionBase *ExpansionBase = NULL;
const struct GfxBase *GfxBase = NULL;
static const struct InputBase *InputBase = NULL;


    /*
     *	Console Stuff
     */

struct ConsoleDesc {
    const char *Name;
    struct Console *(*Probe)(const char *args);
};

static struct ConsoleDesc VideoConsoles[] = {
#if 0
    { "vga", Probe_VGA },
    { "cyber", Probe_Cyber },
    { "retz3", Probe_Retz3 },
#endif
    { "intui", Probe_Intuition },	/* last is default */
};

static struct ConsoleDesc SerialConsoles[] = {
#if 0
    { "ioext", Probe_IOExt },
    { "mfc", Probe_MFC },
#endif
    { "paula", Probe_Paula },		/* last is default */
};

struct Console *VideoConsole;
struct Console *SerialConsole;

const char *VideoName = NULL;
const char *VideoArgs = NULL;
const char *SerialName = NULL;
const char *SerialArgs = NULL;

static struct MsgPort *TimerPort;
static struct timerequest *TimerRequest;
static struct MsgPort *InputPort;
static struct IOStdReq *InputRequest;
static u_long TimeOutSignalMask = 0;
static char FmtBuffer[256], InBuffer[CL_SIZE];


    /*
     *	Map Data
     */

const struct FileVectorData MapVectorData = {
    { LILO_ID, LILO_MAPVECTORID }, MAXVECTORSIZE
};

u_char *MapData = NULL;
u_long MapSize = 0, MapOffset = 0;
struct BootOptions *BootOptions = NULL;
struct BootRecord *BootRecords = NULL;
struct FileDef *Files = NULL;


static u_short MasterMode = 0;
static u_short Auto = 0;
static u_short Aux = 0;
static const char *Prompt = NULL;
static u_short MinKick = 0;
static u_short MaxKick = 65535;


    /*
     *	Linux/m68k Bootstrap Stuff
     */

static const struct BootRecord *DefaultBootRecord;
struct BootData BootData;


    /*
     *	Function Prototypes
     */

u_long Main(struct IOStdReq *bootrequest);
static void ReadMapData(void);
static void SplitNameArgs(const char *option, const char **name,
			  const char **args);
static void InitConsole(void);
static struct Console *FindConsole(const struct ConsoleDesc *consoles,
				   int numconsoles, const char *name,
				   const char *args);
static int Interactive(void);
static void OpenConsole(void);
static void CloseConsole();
static long ReadLine(u_long mode);
static void Sleep(u_long micros);
static int IsAvailable(const struct BootRecord *record);
static void GetBootOS(void);
static u_long BootLinux(const struct BootOptions *global,
			const struct BootData *boot);
static u_long BootNetBSD(struct BootData *boot);


    /*
     *	Main Loader Routine
     *
     *	This routine should return 0 in case of an error (which will cause
     *	a reboot), and non-zero if the user wants to boot AmigaOS.
     *	In all other cases it won't return :-)
     */

u_long Main(struct IOStdReq *bootrequest)
{
    u_long res = 0;
    u_short interactive = 0;

    /* IOStdReq for the Boot Device */
    BootRequest = bootrequest;

    /* Open Libraries */
    SysBase = *(struct ExecBase **)4;
    if (!(ExpansionBase =
	  (struct ExpansionBase *)OpenLibrary("expansion.library", LIB_VERS)))
	Alert(AN_LILO|AG_OpenLib|AO_ExpansionLib);
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",
						  LIB_VERS)))
	Alert(AN_LILO|AG_OpenLib|AO_GraphicsLib);

    /* Open Timer Device */
    if (!(TimerPort = CreateMsgPort()))
	Alert(AN_LILO|AG_NoSignal);
    if (!(TimerRequest = CreateIORequest(TimerPort,
					 sizeof(struct timerequest))))
	Alert(AN_LILO|AG_NoMemory);
    if (OpenDevice("timer.device", UNIT_VBLANK,
		   (struct IORequest *)TimerRequest, 0))
	Alert(AN_LILO|AG_OpenDev|AO_TimerDev);

    /* Open Input Device */
    if (!(InputPort = CreateMsgPort()))
	Alert(AN_LILO|AG_NoSignal);
    if (!(InputRequest = CreateIORequest(InputPort, sizeof(struct IOStdReq))))
	Alert(AN_LILO|AG_NoMemory);
    if (OpenDevice("input.device", 0, (struct IORequest *)InputRequest, 0))
	Alert(AN_LILO|AG_OpenDev|AO_Unknown);
    InputBase = (struct InputBase *)InputRequest->io_Device;

    /* Configuration */
    ReadMapData();
    InitConsole();

    if (!Auto || Interactive()) {
	OpenConsole();
	interactive = 1;
    }

    if (SysBase->Version < MinKick || SysBase->Version > MaxKick) {
	Printf("\nWarning: Kickstart version %ld is out of range %ld-%ld\n\n",
	       SysBase->Version, MinKick, MaxKick);
	if (!interactive) {
	    Puts("Autobooting AmigaOS...\n\n");
	    res = 1;
	    goto done;
	}
    }

    /* If a message is defined, display it now. */
    /* NOT TESTED YET! */
    if (BootOptions->Message)
	Printf( "%s\n", BootOptions->Message );
	
    do {
	DefaultBootRecord = FindBootRecord(NULL);

	if (!DefaultBootRecord) {
	    Puts("\nPanic: No boot Operating System!\n");
	    break;
	}
	FillBootData(DefaultBootRecord, &BootData, 1);
	if (interactive)
	    GetBootOS();

	switch (BootData.OSType) {
	    case BOS_AMIGA:
		Puts("\nBooting AmigaOS...\n\n");
		res = 1;
		break;
	    case BOS_LINUX:
		res = BootLinux(BootOptions, &BootData);
		break;
	    case BOS_NETBSD:
		Puts("\nBooting NetBSD-Amiga...\n\n");
		res = BootNetBSD(&BootData);
		break;
	}
	if (res)
	    break;
	Puts("\nCouldn't boot the desired Operating System\n");
    } while (interactive);

done:
    CloseConsole();
    FreeMapData();

    /* Close Input Device */
    CloseDevice((struct IORequest *)InputRequest);
    DeleteIORequest(InputRequest);
    DeleteMsgPort(InputPort);

    /* Close Timer Device */
    CloseDevice((struct IORequest *)TimerRequest);
    DeleteIORequest(TimerRequest);
    DeleteMsgPort(TimerPort);

    /* Close Libraries */
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)ExpansionBase);

    return(res);
}

    /*
     *  Split an option into Name and Args
     */

static void SplitNameArgs(const char *option, const char **name,
			  const char **args)
{
    char *p = (char *)option;

    *name = p;
    while (*p && *p != ',')
	p++;
    if (*p) {
	*p++ = '\0';
	*args = p;
    }
}


void MachInitDebug( void )
{
    InitConsole();
    OpenConsole();
}


    /*
     *	Read the Map Data
     */

static void ReadMapData(void)
{
    ParseTags();
    
    if (BootOptions->Auto)
	Auto = *BootOptions->Auto;
    if (BootOptions->Aux)
	Aux = *BootOptions->Aux;
    if (BootOptions->Debug)
	Debug = *BootOptions->Debug;
    if (BootOptions->Baud) {
	u_long baud = *BootOptions->Baud;
	if (baud)
	    custom.serper = (5*SysBase->ex_EClockFrequency+baud/2)/baud-1;
    }
    if (BootOptions->KickRange) {
	MinKick = BootOptions->KickRange[0];
	MaxKick = BootOptions->KickRange[1];
    }
    Prompt = BootOptions->Prompt ? BootOptions->Prompt : "Boot image: ";
    if (BootOptions->Video)
	SplitNameArgs(BootOptions->Video, &VideoName, &VideoArgs);
    if (BootOptions->Serial)
	SplitNameArgs(BootOptions->Serial, &SerialName, &SerialArgs);
}


    /*
     *  Initialize the Console
     */

static void InitConsole(void)
{
    static int inited = 0;

    if (inited)
	return;

    VideoConsole = FindConsole(VideoConsoles, arraysize(VideoConsoles),
    			       VideoName, VideoArgs);
    if (Aux)
	SerialConsole = FindConsole(SerialConsoles, arraysize(SerialConsoles),
				    SerialName, SerialArgs);
    inited = 1;
}


    /*
     *  Find an available Console
     */

static struct Console *FindConsole(const struct ConsoleDesc *consoles,
				   int numconsoles, const char *name,
				   const char *args)
{
    struct Console *console = NULL;
    int i;

    for (i = 0; i < numconsoles; i++)
	if ((!strncmp(name, consoles[i].Name, strlen(consoles[i].Name)) ||
	     i == numconsoles-1) && (console = ptr_fkt_offset_jmp(consoles[i].Probe, args)))
	    break;
    return(console);
}


    /*
     *	Any magic keys pressed?
     */

static int Interactive(void)
{
    if (VideoConsole && int_fkt_offset_jmp(VideoConsole->Interactive))
	return(1);
    if (SerialConsole && int_fkt_offset_jmp(SerialConsole->Interactive))
	return(1);
    return(0);
}


    /*
     *	Open the Console
     */

static void OpenConsole(void)
{
    if (VideoConsole)
	int_fkt_offset_jmp(VideoConsole->Open);
    if (SerialConsole)
	int_fkt_offset_jmp(SerialConsole->Open);
}


    /*
     *	Close the Console
     */

static void CloseConsole()
{
    if (VideoConsole)
	int_fkt_offset_jmp(VideoConsole->Close);
    if (SerialConsole)
	int_fkt_offset_jmp(SerialConsole->Close);
}


    /*
     *	Print a String to the Console
     */

void Puts(const char *str)
{
    if (VideoConsole)
	int_fkt_offset_jmp(VideoConsole->Puts, str);
    if (SerialConsole)
	int_fkt_offset_jmp(SerialConsole->Puts, str);
}


    /*
     *	Put a Character to the Console
     */

void PutChar(int c)
{
    if (VideoConsole)
	int_fkt_offset_jmp(VideoConsole->PutChar,c);
    if (SerialConsole)
	int_fkt_offset_jmp(SerialConsole->PutChar,c);
}


    /*
     *	Get a Character from the Console
     */

int GetChar(void)
{
    int res = -1;

    if (VideoConsole || SerialConsole) {
	if (VideoConsole)
	    int_fkt_offset_jmp(VideoConsole->PreGetChar);
	if (SerialConsole)
	    int_fkt_offset_jmp(SerialConsole->PreGetChar);
	do {
	    if (VideoConsole && (res = int_fkt_offset_jmp(VideoConsole->GetChar)) != -1)
		break;
	    if (SerialConsole && (res = int_fkt_offset_jmp(SerialConsole->GetChar)) != -1)
		break;
	} while (MasterMode || Debug || !TimedOut());
	if (VideoConsole)
	    int_fkt_offset_jmp(VideoConsole->PostGetChar);
	if (SerialConsole)
	    int_fkt_offset_jmp(SerialConsole->PostGetChar);
    }
    return(res);
}


    /*
     *	Format and Print a String to the Console
     */

asm(".text
	.align 4\n"
SYMBOL_NAME_STR(PutCharInBuf) ":
	movel	%a0,%sp@-
	lea	%pc@(" SYMBOL_NAME_STR(FmtBuffer) "+255),%a0
	cmpl	%a0,%a3
	jne	1f
	clrb	%a3@
	jra	2f
1:	moveb	%d0,%a3@+
2:	movel	%sp@+,%a0
	rts
");

extern void *PutCharInBuf(char c);

void Printf(const char *fmt, ...)
{
    va_list args;
    static char fmt2[256];
    int i, j, fmtesc = 0;

    va_start(args, fmt);
    /*
     *  Compensate for the 16-bit nature of RawDoFmt()
     */
    for (i = 0, j = 0; fmt[i] && j < 255; i++, j++) {
	if (fmtesc)
	    switch (fmt[i]) {
		case 'd':       /* decimal */
		case 'u':       /* unsigned decimal (the autodoc incorrectly */
				/* states that %u is always 32-bit) */
		case 'x':       /* hexadecimal */
		case 'c':       /* character */
		    if (fmt[i-1] != 'l')
			fmt2[j++] = 'l';
		    if (j == 255)
			goto full;
		case 'b':       /* BSTR */
		case 's':       /* string */
		case '%':       /* % */
		    fmtesc = 0;
		case '-':       /* left justification */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '.':
		case 'l':       /* 32 bit flag */
		    break;
#if 1 /* debugging */
		default:
		    Puts("Bad format string `");
		    Puts(fmt);
		    Puts("'\n");
		    break;
#endif /* debugging */
	    }
	else if (fmt[i] == '%')
	    fmtesc = 1;
	fmt2[j] = fmt[i];
    }
full:
    fmt2[j] = '\0';
    RawDoFmt(fmt2, args, (void (*)())PutCharInBuf, FmtBuffer);
    Puts(FmtBuffer);
    va_end(args);
}


    /*
     *	Read a Line from the Console
     */

static long ReadLine(u_long mode)
{
    long c;
    long length = 0;
    u_long eol = 0;

    while (!eol && ((c = GetChar()) != -1))
	switch (c) {
	    case '\r':
	    case '\n':
		InBuffer[length] = '\0';
		if ((mode == ECHO_NORMAL) || (mode == ECHO_DOT))
		    PutChar('\n');
		eol = 1;
		break;
	    case '\b':
	    case 0x7f:
		if (length > 0) {
		    length--;
		    if ((mode == ECHO_NORMAL) || (mode == ECHO_DOT))
			Puts("\b \b");
		}
		break;
	    case '\t':
		if (!MasterMode) {
		    strcpy(InBuffer, "help");
		    length = strlen("help");
		    switch (mode) {
			case ECHO_NORMAL:
			    Puts("help\n");
			    break;
			case ECHO_DOT:
			    Puts("++++\n");
			    break;
		    }
		    eol = 1;
		    break;
		}
	    default:
		if (length < sizeof(InBuffer)-1) {
		    InBuffer[length++] = c;
		    switch (mode) {
			case ECHO_NORMAL:
			    PutChar(c);
			    break;
			case ECHO_DOT:
			    PutChar('+');
			    break;
		    }
		}
		break;
	}
    return(eol ? length : -1);
}


    /*
     *  Read the Command Line
     */

char *ReadCommandLine(void)
{
    ReadLine(ECHO_NORMAL);
    return(InBuffer);
}


    /*
     *	Set the Time Out Value
     */

void SetTimeOut(u_long seconds)
{
    if (!TimeOutSignalMask) {
	TimerRequest->io_Command = TR_ADDREQUEST;
	TimerRequest->tv_secs = seconds;
	TimerRequest->tv_micro = 0;
	SendIO((struct IORequest *)TimerRequest);
	TimeOutSignalMask = 1<<TimerPort->mp_SigBit;
    }
}


    /*
     *	Check for a Time Out
     */

int TimedOut(void)
{
    return(TimeOutSignalMask && CheckIO((struct IORequest *)TimerRequest));
}


    /*
     *	Clear Time Out
     */

void ClearTimeOut(void)
{
    if (TimeOutSignalMask) {
	AbortIO((struct IORequest *)TimerRequest);
	WaitIO((struct IORequest *)TimerRequest);
	TimeOutSignalMask = 0;
    }
}


    /*
     *	Sleep for a Specified Period
     */

static void Sleep(u_long micros)
{
    TimerRequest->io_Command = TR_ADDREQUEST;
    TimerRequest->io_Flags = IOF_QUICK;
    TimerRequest->tv_secs = micros/1000000;
    TimerRequest->tv_micro = micros%1000000;
    DoIO((struct IORequest *)TimerRequest);
}


    /*
     *  Check whether a Boot Image is available
     */

static int IsAvailable(const struct BootRecord *record)
{
    u_long type = record->OSType ? *record->OSType : BOS_LINUX;

    if (type != BOS_AMIGA) {
	if (!FindVector(record->Kernel))
	    return(0);
	if (record->Ramdisk && !FindVector(record->Ramdisk))
	    return(0);
    }
    return(1);
}


    /*
     *  List all available Boot Records
     */

void ListRecords(void)
{
    const struct BootRecord *record;

    Puts("\nBoot images:\n");
    for (record = BootRecords; record; record = record->Next) {
	if (record == DefaultBootRecord)
	    Puts("\e[1m");
	Printf("    %-31.31s (", record->Label);
	if (record->Alias)
	    Printf("`%s', ", record->Alias);
	switch (record->OSType ? *record->OSType : BOS_LINUX) {
	    case BOS_AMIGA:
		Puts("AmigaOS");
		break;
	    case BOS_LINUX:
		Puts("Linux/m68k");
		break;
	    case BOS_NETBSD:
		Puts("NetBSD-Amiga");
		break;
	    default:
		Puts("Unknown");
		break;
	}
	if (record->Password)
	    Puts(", Restricted");
	if (!IsAvailable(record))
	    Puts(", N/A");
	Puts(")\n");
	if (record == DefaultBootRecord)
	    Puts("\e[0m");
    }
}


    /*
     *  List all available Files
     */

void ListFiles(void)
{
    const struct FileDef *file;

    Puts("\nFiles:\n");
    for (file = Files; file; file = file->Next) {
	Printf("    %s (", file->Path);
	if (file->Vector)
	    Printf("%ld bytes", file->Vector[0]);
	else
	    Puts("N/A");
	Puts(")\n");
    }
}


    /*
     *	Ask the User for a Boot Operating System
     */

static void GetBootOS(void)
{
    const struct BootRecord *record;

    if (BootOptions->TimeOut ? *BootOptions->TimeOut : 20)
	SetTimeOut(*BootOptions->TimeOut);

    while (1) {
	PutChar('\n');
	Puts(Prompt);
	if ((ReadLine(ECHO_NORMAL)) < 1)
	    break;
	if (!strcmp(InBuffer, "help") || !strcmp(InBuffer, "?")) {
	    Printf("\n%s\nValid commands: su, help, ?\n", LiloVersion);
	    ListRecords();
	    PutChar('\n');
#if 0
	} else if (!strcmp(InBuffer, "do test")) {
	    extern void do_test(void);
	    do_test();
#endif
	} else if (!strcmp(InBuffer, "su")) {
	    if (BootOptions->MasterPassword) {
		Puts("\nPassword: ");
		if (((ReadLine(ECHO_DOT)) < 1) ||
		     strcmp(InBuffer, BootOptions->MasterPassword)) {
		    Puts("\nIncorrect password.\n");
		    continue;
		}
	    }
	    MasterMode = 1;
	    Monitor();
	    break;
	} else {
	    if (!(record = FindBootRecord(InBuffer))) {
		Printf("\nNonexistent boot image `%s'.\n", InBuffer);
		continue;
	    }
	    if (!IsAvailable(record)) {
		Printf("\nBoot image `%s' is not available.\n", InBuffer);
		continue;
	    } else if (record->Password) {
		Puts("\n\nPassword: ");
		if (((ReadLine(ECHO_DOT)) < 1) ||
		    strcmp(InBuffer, record->Password)) {
		    Puts("\nIncorrect password.\n");
		    continue;
		}
	    }
	    FillBootData(record, &BootData, 0);
	    break;
	}
    }
    ClearTimeOut();
}


    /*
     *  Fill the Boot Data according to a Boot Record
     */

void FillBootData(const struct BootRecord *record, struct BootData *boot,
		  int autoboot)
{
    int i;

    boot->OSType = record->OSType ? *record->OSType : BOS_LINUX;
    if (record->Kernel) {
	strncpy(boot->Kernel, record->Kernel, PATH_SIZE);
	boot->Kernel[PATH_SIZE-1] = '\0';
    } else
	boot->Kernel[0] = '\0';
    if (record->Ramdisk) {
	strncpy(boot->Ramdisk, record->Ramdisk, PATH_SIZE);
	boot->Ramdisk[PATH_SIZE-1] = '\0';
    } else
	boot->Ramdisk[0] = '\0';
    if (record->Args)
	strncpy(boot->Args, record->Args, CL_SIZE);
    else
	boot->Args[0] = '\0';
    strncat(boot->Args, " BOOT_IMAGE=", CL_SIZE);
    strncat(boot->Args, record->Label, CL_SIZE);
    if (autoboot)
	strncat(boot->Args, " auto", CL_SIZE);
    boot->Args[CL_SIZE-1] = '\0';
    boot->Debug = Debug;
    boot->Model = record->Model ? *record->Model : AMI_UNKNOWN;
    boot->ChipSize = record->ChipSize ? *record->ChipSize : 0;
    boot->NumMemory = 0;
    for (i = 0; i < NUM_MEMINFO; i++)
	if (record->FastChunks[i]) {
	    boot->FastChunks[i].Address = record->FastChunks[i][0];
	    boot->FastChunks[i].Size = record->FastChunks[i][1];
	    boot->NumMemory++;
	}
}


    /*
     *  Boot the Linux/m68k Operating System
     */

static u_long BootLinux(const struct BootOptions *global,
			const struct BootData *boot)
{
    struct linuxboot_args args;
    int processor, i, j;

    Puts("\nBooting Linux/m68k...\n\n");

    memset(&args.bi, 0, sizeof(args.bi));
    processor = boot->Processor ? boot->Processor
				: global->Processor ? *global->Processor : 0;
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
    if (boot->NumMemory) {
	for (i = 0, j = 0; i < boot->NumMemory; i++)
	    if (boot->FastChunks[i].Size) {
		args.bi.memory[j].addr = boot->FastChunks[i].Address;
		args.bi.memory[j].size = boot->FastChunks[i].Size;
		j++;
	    }
	if (j)
	    args.bi.num_memory = j;
    } else if (global->FastChunks[0])
	for (i = 0; global->FastChunks[i]; i++) {
	    args.bi.memory[i].addr = global->FastChunks[i][0];
	    args.bi.memory[i].size = global->FastChunks[i][1];
	}
    strncpy(args.bi.command_line, boot->Args, CL_SIZE);
    args.bi.command_line[CL_SIZE-1] = '\0';
    if (boot->Model != AMI_UNKNOWN)
	args.bi.model = boot->Model;
    else if (global->Model)
	args.bi.model = *global->Model;
    if (boot->ChipSize)
	args.bi.chip_size = boot->ChipSize;
    else if (global->ChipSize)
	args.bi.chip_size = *global->ChipSize;

    args.kernelname = boot->Kernel[0] ? boot->Kernel : NULL;
    args.ramdiskname = boot->Ramdisk[0] ? boot->Ramdisk : NULL;
    args.debugflag = boot->Debug;
    args.keep_video = 0;
    args.reset_boards = 0;
    args.baud = BootOptions->Baud ? *BootOptions->Baud : 0;
    args.sleep = Sleep;

    /* Do The Right Stuff */
    return(linuxboot(&args));
}


    /*
     *  Boot the NetBSD-Amiga Operating System
     */

static u_long BootNetBSD(struct BootData *boot)
{
    Puts("Not yet implemented :-)\n");
    return(0);
}


    /*
     *	Read one or more Sectors from the Boot Device
     */

long ReadSectors( char *buf, unsigned int device, unsigned int sector,
		  unsigned int cnt )
{
    long err;

    BootRequest->io_Command = CMD_READ;
    BootRequest->io_Flags = IOF_QUICK;
    BootRequest->io_Length = cnt*HARD_SECTOR_SIZE;
    BootRequest->io_Data = buf;
    BootRequest->io_Offset = sector*HARD_SECTOR_SIZE;
    err = DoIO((struct IORequest *)BootRequest);

    BootRequest->io_Command = TD_MOTOR;
    BootRequest->io_Flags = IOF_QUICK;
    BootRequest->io_Length = 0;
    DoIO((struct IORequest *)BootRequest);

    return err;
}


    /*
     *  Integer Math
     *
     *  Libnix uses utility.library if these aren't present
     */

asm("
		.globl	___umodsi3
		.globl	___udivsi3
		.globl	___udivsi4
		.globl	___mulsi3

| D0.L = D0.L % D1.L unsigned

___umodsi3:	moveml	%sp@(4:w),%d0/%d1
		jbsr	___udivsi4
		movel	%d1,%d0
		rts

| D0.L = D0.L / D1.L unsigned

___udivsi3:	moveml	%sp@(4:w),%d0/%d1
___udivsi4:	movel	%d3,%sp@-
		movel	%d2,%sp@-
		movel	%d1,%d3
		swap	%d1
		tstw	%d1
		jne	1f
		movew	%d0,%d2
		clrw	%d0
		swap	%d0
		divu	%d3,%d0
		movel	%d0,%d1
		swap	%d0
		movew	%d2,%d1
		divu	%d3,%d1
		movew	%d1,%d0
		clrw	%d1
		swap	%d1
		jra	4f
1:		movel	%d0,%d1
		swap	%d0
		clrw	%d0
		clrw	%d1
		swap	%d1
		moveq	#16-1,%d2
2:		addl	%d0,%d0
		addxl	%d1,%d1
		cmpl	%d1,%d3
		jhi	3f
		subl	%d3,%d1
		addqw	#1,%d0
3:		dbra	%d2,2b
4:		movel	%sp@+,%d2
		movel	%sp@+,%d3
		rts

| D0 = D0 * D1

___mulsi3:	moveml	%sp@(4:w),%d0/%d1
		movel	%d3,%sp@-
		movel	%d2,%sp@-
		movew	%d1,%d2
		mulu	%d0,%d2
		movel	%d1,%d3
		swap	%d3
		mulu	%d0,%d3
		swap	%d3
		clrw	%d3
		addl	%d3,%d2
		swap	%d0
		mulu	%d1,%d0
		swap	%d0
		clrw	%d0
		addl	%d2,%d0
		movel	%sp@+,%d2
		movel	%sp@+,%d3
		rts
");


void exit(int res)
{
    Alert(AN_LILO);	/* Any better idea? */
    while (1);
}
