/*
 *  Amiga Linux/m68k Loader -- Paula Console (builtin serial port)
 *
 *  © Copyright 1997 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: con_paula.c,v 1.1 1997-08-12 15:27:02 rnhodek Exp $
 * 
 * $Log: con_paula.c,v $
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


    /*
     *  Function Prototypes
     */

struct Console *Probe_Paula(const char *args);

static int Con_Attn(void);
static void Con_Open(void);
static void Con_Close(void);
static void Con_Puts(const char *s);
static void Con_PutChar(char c);
static void Con_PreGetChar(void);
static int Con_GetChar(void);
static void Con_PostGetChar(void);

static void InstallBreakHandler(void);
static void RemoveBreakHandler(void);


    /*
     *  Console Functions
     */

static struct Console Console = {
    Con_Attn, Con_Open, Con_Close, Con_Puts, Con_PutChar, Con_PreGetChar,
    Con_GetChar, Con_PostGetChar, 0
};


    /*
     *  Do we exist?
     */

struct Console *Probe_Paula(const char *args)
{
    return(&Console);
}


    /*
     *	Any magic keys pressed?
     */

static int Con_Attn(void)
{
    u_long res = -1;

    InstallBreakHandler();
    SetTimeOut(1);
    do
	if (RawMayGetChar() != -1) {
	    res = 1;
	    while (RawMayGetChar() != -1);
	    break;
	}
    while (!TimedOut());
    ClearTimeOut();
    RemoveBreakHandler();
    return(res);
}


    /*
     *	Open the Console
     */

static void Con_Open(void)
{
    if (!Console.Opened) {
	InstallBreakHandler();
	Console.Opened = 1;
    }
}


    /*
     *	Close the Console
     */

static void Con_Close(void)
{
    if (Console.Opened) {
	RemoveBreakHandler();
	Console.Opened = 0;
    }
}


    /*
     *	Print a String to the Console
     */

static void Con_Puts(const char *s)
{
    while (*s)
	RawPutChar(*s++);
}


    /*
     *	Put a Character to the Console
     */

static void Con_PutChar(char c)
{
    RawPutChar(c);
}


    /*
     *	Get a Character from the Console
     */

static void Con_PreGetChar(void)
{}

static int Con_GetChar(void)
{
    return(RawMayGetChar());
}

static void Con_PostGetChar(void)
{}


    /*
     *	Replacement for exec.library/Debug()
     *
     *	We don't want to meet a debugger when DEL is pressed on the terminal
     */

asm(".text
	.align 4\n"
SYMBOL_NAME_STR(NewDebug) ":
	rts
");

static void *OldDebug = NULL;
extern void NewDebug(void);

static void InstallBreakHandler(void)
{
    if (!OldDebug)
	OldDebug = SetFunction((struct Library *)SysBase, LVODebug, NewDebug);
}

static void RemoveBreakHandler(void)
{
    if (OldDebug) {
	SetFunction((struct Library *)SysBase, LVODebug, OldDebug);
	OldDebug = NULL;
    }
}
