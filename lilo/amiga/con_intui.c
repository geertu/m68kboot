/*
 *  Amiga Linux/m68k Loader -- Intuition Console (PAL/NTSC screen)
 *
 *  © Copyright 1997 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: con_intui.c,v 1.1 1997-08-12 15:27:02 rnhodek Exp $
 * 
 * $Log: con_intui.c,v $
 * Revision 1.1  1997-08-12 15:27:02  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <stdlib.h>
#include <sys/types.h>

#include "console.h"
#include "amigaos.h"
#include "loader.h"


static const struct IntuitionBase *IntuitionBase = NULL;
static const struct InputBase *InputBase = NULL;


static u_char Title[] = "Amiga Linux/m68k Loader";

static struct NewScreen NewScreen = {
    0, 0, 0, 0, 2, 0, 1, HIRES, CUSTOMSCREEN, NULL, NULL, NULL, NULL
};
static struct NewWindow NewWindow = {
    0, 0, 0, 0, 2, 3, 0, WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE, NULL, NULL,
    NULL, NULL, NULL, 0, 0, 0, 0, CUSTOMSCREEN
};
static struct Screen *Screen;
static struct Window *Window;
static struct MsgPort *ConsolePort;
static struct IOStdReq *ConsoleRequest = NULL;


    /*
     *  Function Prototypes
     */

struct Console *Probe_Intuition(const char *args);

static int Con_Interactive(void);
static void Con_Open(void);
static void Con_Close(void);
static void Con_Puts(const char *s);
static void Con_PutChar(char c);
static void Con_PreGetChar(void);
static int Con_GetChar(void);
static void Con_PostGetChar(void);


    /*
     *  Console Functions
     */

static struct Console Console = {
    Con_Interactive, Con_Open, Con_Close, Con_Puts, Con_PutChar,
    Con_PreGetChar, Con_GetChar, Con_PostGetChar, 0
};


    /*
     *  Do we exist?
     */

struct Console *Probe_Intuition(const char *args)
{
    return(&Console);
}


    /*
     *	Any magic keys pressed?
     */

static int Con_Interactive(void)
{
#ifdef V36_COMPATIBILITY
    if (SysBase->Version < 37) {
	/*
	 *  V36 (A3000 Beta ROM) compatible replacement for PeekQualifier()
	 */
	const struct KeyboardBase *KeyboardBase = NULL;
	struct MsgPort *KeyboardPort;
	struct IOStdReq *KeyboardRequest;
	u_char KeyboardMatrix[13];
	u_short keys;

	/* Open Keyboard Device */

	if (!(KeyboardPort = CreateMsgPort()))
	    Alert(AN_LILO|AG_NoSignal);
	if (!(KeyboardRequest = CreateIORequest(KeyboardPort,
						sizeof(struct IOStdReq))))
	    Alert(AN_LILO|AG_NoMemory);
	if (OpenDevice("keyboard.device", 0,
		       (struct IORequest *)KeyboardRequest, 0))
	    Alert(AN_LILO|AG_OpenDev|AO_KeyboardDev);
	KeyboardBase = (struct KeyboardBase *)KeyboardRequest->io_Device;

	KeyboardRequest->io_Command = KBD_READMATRIX;
	KeyboardRequest->io_Flags = IOF_QUICK;
	KeyboardRequest->io_Length = sizeof(KeyboardMatrix);
	KeyboardRequest->io_Data = KeyboardMatrix;
	DoIO((struct IORequest *)KeyboardRequest);

	keys = KeyboardMatrix[12];
	if (!(custom.potgor & 0x0100))
	    keys |= IEQUALIFIER_MIDBUTTON;
	if (!(custom.potgor & 0x0400))
	    keys |= IEQUALIFIER_RBUTTON;
	if (!(ciaa.pra & 0x40))
	    keys |= IEQUALIFIER_LEFTBUTTON;

	/* Close Keyboard Device */

	CloseDevice((struct IORequest *)KeyboardRequest);
	DeleteIORequest(KeyboardRequest);
	DeleteMsgPort(KeyboardPort);

	return(keys);
    } else
#endif /* V36_COMPATIBILITY */
	return(PeekQualifier());
}


    /*
     *	Open the Console
     */

static void Con_Open(void)
{
    if (!Console.Opened) {
	if (!(IntuitionBase =
	      (struct IntuitionBase *)OpenLibrary("intuition.library",
	      					  LIB_VERS)))
	    Alert(AN_LILO|AG_OpenLib|AO_Intuition);
	NewScreen.Width = GfxBase->NormalDisplayColumns;
	NewScreen.Height = GfxBase->NormalDisplayRows;
	NewScreen.DefaultTitle = Title;
	if (!(Screen = OpenScreen(&NewScreen)))
	    Alert(AN_LILO|AN_OpenScreen);
	NewWindow.Width = NewScreen.Width;
	NewWindow.Height = NewScreen.Height;
	NewWindow.Screen = Screen;
	NewWindow.Title = Title;
	if (!(Window = OpenWindow(&NewWindow)))
	    Alert(AN_LILO|AN_OpenWindow);
	if (!(ConsolePort = CreateMsgPort()))
	    Alert(AN_LILO|AG_NoSignal);
	if (!(ConsoleRequest = CreateIORequest(ConsolePort,
					       sizeof(struct IOStdReq))))
	    Alert(AN_LILO|AG_NoMemory);
	ConsoleRequest->io_Data = Window;
	if (OpenDevice("console.device", CONU_SNIPMAP,
		       (struct IORequest *)ConsoleRequest, CONFLAG_DEFAULT))
	    Alert(AN_LILO|AG_OpenDev|AO_ConsoleDev);
	Console.Opened = 1;
    }
};


    /*
     *	Close the Console
     */

static void Con_Close(void)
{
    if (Console.Opened) {
	CloseDevice((struct IORequest *)ConsoleRequest);
	DeleteIORequest(ConsoleRequest);
	DeleteMsgPort(ConsolePort);
	CloseWindow(Window);
	CloseScreen(Screen);
	CloseLibrary((struct Library*)IntuitionBase);
	Console.Opened = 0;
    }
}


    /*
     *	Print a String to the Console
     */

static void Con_Puts(const char *s)
{
    ConsoleRequest->io_Command = CMD_WRITE;
    ConsoleRequest->io_Length = -1;
    ConsoleRequest->io_Data = (char *)s;
    SendIO((struct IORequest *)ConsoleRequest);
    WaitIO((struct IORequest *)ConsoleRequest);
}


    /*
     *	Put a Character to the Console
     */

static void Con_PutChar(char c)
{
    ConsoleRequest->io_Command = CMD_WRITE;
    ConsoleRequest->io_Length = 1;
    ConsoleRequest->io_Data = &c;
    SendIO((struct IORequest *)ConsoleRequest);
    WaitIO((struct IORequest *)ConsoleRequest);
}


    /*
     *	Get a Character from the Console
     */

static void Con_PreGetChar(void)
{
    static char c;

    ConsoleRequest->io_Command = CMD_READ;
    ConsoleRequest->io_Length = 1;
    ConsoleRequest->io_Data = &c;
    SendIO((struct IORequest *)ConsoleRequest);
}

static int Con_GetChar(void)
{
    if (CheckIO((struct IORequest *)ConsoleRequest))
	return(*(char *)ConsoleRequest->io_Data);
    else
	return(-1);
}

static void Con_PostGetChar(void)
{
    AbortIO((struct IORequest *)ConsoleRequest);
    WaitIO((struct IORequest *)ConsoleRequest);
}
