/*
 *  Amiga Linux/m68k Loader -- AmigaOS API Definitions
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: amigaos.h,v 1.4 1998-04-06 01:40:56 dorchain Exp $
 * 
 * $Log: amigaos.h,v $
 * Revision 1.4  1998-04-06 01:40:56  dorchain
 * make loader linux-elf.
 * made amiga bootblock working again
 * compiled, but not tested bootstrap
 * loader breaks with MapOffset problem. Stack overflow?
 *
 * Revision 1.3  1998/02/19 20:40:13  rnhodek
 * Make things compile again
 *
 * Revision 1.2  1997/09/19 09:06:50  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:26:59  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _amigaos_h
#define _amigaos_h


    /*
     *  linuxboot.h contains some more AmigaOS definitions
     */

#ifndef __ASSEMBLY__
#include "linuxboot.h"
#include "inline-funcs.h"
#endif /* !__ASSEMBLY__ */


    /*
     *	Exec Library Definitions
     */

#define AT_DeadEnd		0x80000000
#define AG_NoMemory		0x00010000
#define AG_OpenLib		0x00030000
#define AG_OpenDev		0x00040000
#define AG_OpenRes		0x00050000
#define AG_IOError		0x00060000
#define AG_NoSignal		0x00070000
#define AG_BadParm		0x00080000
#define AO_GraphicsLib		0x00008002
#define AO_Intuition		0x00008004
#define AO_ExpansionLib		0x0000800A
#define AO_ConsoleDev		0x00008011
#define AO_KeyboardDev		0x00008013
#define AO_TimerDev		0x00008015
#define AO_Unknown		0x00008035
#define AN_OpenScreen		0x84010007
#define AN_OpenWindow		0x8401000B

#define CMD_READ		2
#define CMD_WRITE		3
#define TD_MOTOR		9
#define KBD_READMATRIX		10

#define IOF_QUICK		1

#define UNIT_VBLANK		1
#define TR_ADDREQUEST		9

#ifndef MEMF_PUBLIC
#define MEMF_PUBLIC		1
#endif
#ifndef MEMF_CLEAR
#define MEMF_CLEAR		0x10000
#endif

#define CONU_SNIPMAP		3
#define CONFLAG_DEFAULT		0

#define IEQUALIFIER_LSHIFT	0x0001
#define IEQUALIFIER_RSHIFT	0x0002
#define IEQUALIFIER_CAPSLOCK	0x0004
#define IEQUALIFIER_CONTROL	0x0008
#define IEQUALIFIER_LALT	0x0010
#define IEQUALIFIER_RALT	0x0020
#define IEQUALIFIER_LCOMMAND	0x0040
#define IEQUALIFIER_RCOMMAND	0x0080
#define IEQUALIFIER_MIDBUTTON	0x1000
#define IEQUALIFIER_RBUTTON	0x2000
#define IEQUALIFIER_LEFTBUTTON	0x4000

#ifndef __ASSEMBLY__

struct MsgPort {
    u_char fill1[15];
    u_char mp_SigBit;
    u_char fill2[18];
};

struct IOStdReq {
    u_char fill1[20];
    struct Device *io_Device;
    u_char fill2[4];
    u_short io_Command;
    u_char io_Flags;
    char io_Error;
    u_long io_Actual;
    u_long io_Length;
    void *io_Data;
    u_long io_Offset;
};

struct timerequest {
    u_char fill1[28];
    u_short io_Command;
    u_char io_Flags;
    u_char fill2[1];
    u_long tv_secs;
    u_long tv_micro;
};

struct IORequest;
struct Library;

#else /* __ASSEMBLY__ */

#define IO_COMMAND		28
#define IO_FLAGS		30
#define IO_LENGTH		36
#define IO_DATA			40
#define IO_OFFSET		44

#define IOEXTTD_SIZE		56

#define LIB_VERSION		20

#define RT_INIT			22

#define eb_Flags		34
#define EBB_SILENTSTART		6

#endif /* __ASSEMBLY__ */


    /*
     *	DOS Library Definitions
     */

#define HUNK_CODE		1001
#define HUNK_DATA		1002
#define HUNK_BSS		1003
#define HUNK_RELOC32		1004
#define HUNK_END		1010
#define HUNK_HEADER		1011


    /*
     *	Intuition Library Definitions
     */

#define CUSTOMSCREEN		0x000f
#ifndef HIRES
#define HIRES			0x8000
#endif

#define WFLG_SIMPLE_REFRESH	0x00000040
#define WFLG_ACTIVATE		0x00001000

#ifndef __ASSEMBLY__
struct NewScreen {
    short LeftEdge;
    short TopEdge;
    short Width;
    short Height;
    short Depth;
    u_char DetailPen;
    u_char BlockPen;
    u_short ViewModes;
    u_short Type;
    struct TextAttr *Font;
    u_char *DefaultTitle;
    struct Gadget *Gadgets;
    struct BitMap *CustomBitMap;
};

struct NewWindow {
    short LeftEdge;
    short TopEdge;
    short Width;
    short Height;
    u_char DetailPen;
    u_char BlockPen;
    u_long IDCMPFlags;
    u_long Flags;
    struct Gadget *FirstGadget;
    struct Image *CheckMark;
    u_char *Title;
    struct Screen *Screen;
    struct BitMap *BitMap;
    short MinWidth;
    short MinHeight;
    u_short MaxWidth;
    u_short MaxHeight;
    u_short Type;
};

struct Window;
#endif /* !__ASSEMBLY__ */


    /*
     *	Amiga Shared Library/Device Functions
     */

#define LVOAbortIO		-0x1e0
#define LVOAlert		-0x6c
#ifndef LVOAllocVec
#define LVOAllocVec		-0x2ac
#endif
#define LVOCacheClearU		-0x27c
#define LVOCheckIO		-0x1d4
#define LVOCloseDevice		-0x1c2
#define LVOCloseLibrary		-0x19e
#define LVOCreateIORequest	-0x28e
#define LVOCreateMsgPort	-0x29a
#define LVODebug		-0x72
#define LVODeleteIORequest	-0x294
#define LVODeleteMsgPort	-0x2a0
#define LVODoIO			-0x1c8
#ifndef LVOFindResident
#define LVOFindResident		-0x60
#endif
#ifndef LVOFreeVec
#define LVOFreeVec		-0x2b2
#endif
#define LVOOpenDevice		-0x1bc
#define LVOOpenLibrary		-0x228
#define LVORawDoFmt		-0x20a
#define LVORawMayGetChar	-0x1fe
#define LVORawPutChar		-0x204
#define LVOSendIO		-0x1ce
#define LVOSetFunction		-0x1a4
#define LVOSuperVisor		-0x1e
#define LVOWaitIO		-0x1da

#define LVOPeekQualifier	-0x2a

#define LVODisBlitter		-0x1ce
#define LVOOwnBlitter		-0x1c8

#define LVOCloseScreen		-0x42
#define LVOCloseWindow		-0x48
#define LVOOpenScreen		-0xc6
#define LVOOpenWindow		-0xcc

#ifndef __ASSEMBLY__
extern const struct ExecBase *SysBase;

static __inline char AbortIO(struct IORequest *ioRequest)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1e0);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void Alert(u_long alertNum)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register u_long d7 __asm("d7") = alertNum;

    __asm __volatile ("jsr %%a6@(-0x6c);"
		      : /* no output */
		      : "r" (a6), "r" (d7)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline u_short CheckIO(struct IORequest *ioRequest)
{
    register u_short _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1d4);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void CloseDevice(struct IORequest *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1c2);"
		      : /* no output */
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void CloseLibrary(struct Library *library)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct Library *a1 __asm("a1") = library;

    __asm __volatile ("jsr %%a6@(-0x19e);"
		      : /* no output */
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void *CreateIORequest(struct MsgPort *port, u_long size)
{
    register struct Library *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;
    register u_long d0 __asm("d0") = size;

    __asm __volatile ("jsr %%a6@(-0x28e);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline struct MsgPort *CreateMsgPort(void)
{
    register struct MsgPort *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;

    __asm __volatile ("jsr %%a6@(-0x29a);"
		      : "=r" (_res)
		      : "r" (a6)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void DeleteIORequest(void *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register void *a0 __asm("a0") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x294);"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void DeleteMsgPort(struct MsgPort *port)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct MsgPort *a0 __asm("a0") = port;

    __asm __volatile ("jsr %%a6@(-0x2a0);"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline char DoIO(struct IORequest *ioRequest)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1c8);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline char OpenDevice(const u_char *devName, u_long unit,
				struct IORequest *ioRequest, u_long flags)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register const u_char *a0 __asm("a0") = devName;
    register u_long d0 __asm("d0") = unit;
    register struct IORequest *a1 __asm("a1") = ioRequest;
    register u_long d1 __asm("d1") = flags;

    __asm __volatile ("jsr %%a6@(-0x1bc);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (a1), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline struct Library *OpenLibrary(const u_char *libName,
					    u_long version)
{
    register struct Library *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register const u_char *a1 __asm("a1") = libName;
    register u_long d0 __asm("d0") = version;

    __asm __volatile ("jsr %%a6@(-0x228);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void *RawDoFmt(const u_char *formatString, void *dataStream,
			       void (*putChProc)(), void *putChData)
{
    register void *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register const u_char *a0 __asm("a0") = formatString;
    register void *a1 __asm("a1") = dataStream;
    register void (*a2)() __asm("a2") = putChProc;
    register void *a3 __asm("a3") = putChData;

    __asm __volatile ("jsr %%a6@(-0x20a);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0), "r" (a1), "r" (a2), "r" (a3)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline long RawMayGetChar(void)
{
    register long _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;

    __asm __volatile ("jsr %%a6@(-0x1fe);"
		      : "=r" (_res)
		      : "r" (a6)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void RawPutChar(char c)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register char d0 __asm("d0") = c;

    __asm __volatile ("jsr %%a6@(-0x204);"
		      : /* no output */
		      : "r" (a6), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void SendIO(struct IORequest *ioRequest)
{
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1ce);"
		      : /* no output */
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void *SetFunction(struct Library *library, long funcOffset,
				  void *funcEntry)
{
    register void *_res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct Library *a1 __asm("a1") = library;
    register long a0 __asm("a0") = funcOffset;
    register void *d0 __asm("d0") = funcEntry;

    __asm __volatile ("jsr %%a6@(-0x1a4);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1), "r" (a0), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline char WaitIO(struct IORequest *ioRequest)
{
    register char _res __asm("d0");
    register const struct ExecBase *a6 __asm("a6") = SysBase;
    register struct IORequest *a1 __asm("a1") = ioRequest;

    __asm __volatile ("jsr %%a6@(-0x1da);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct InputBase *InputBase;

static __inline u_short PeekQualifier(void)
{
    register u_short _res __asm("d0");
    register const struct InputBase *a6 __asm("a6") = InputBase;

    __asm __volatile ("jsr %%a6@(-0x2a);"
		      : "=r" (_res)
		      : "r" (a6)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct GfxBase *GfxBase;

static __inline void DisOwnBlitter(void)
{
    register const struct GfxBase *a6 __asm("a6") = GfxBase;

    __asm __volatile ("jsr %%a6@(-0x1ce);"
		      : /* no output */
		      : "r" (a6)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void OwnBlitter(void)
{
    register const struct GfxBase *a6 __asm("a6") = GfxBase;

    __asm __volatile ("jsr %%a6@(-0x1c8);"
		      : /* no output */
		      : "r" (a6)
		      : "a0", "a1", "d0", "d1", "memory");
}


extern const struct IntuitionBase *IntuitionBase;

static __inline u_short CloseScreen(struct Screen *screen)
{
    register u_short _res __asm("d0");
    register const struct IntuitionBase *a6 __asm("a6") = IntuitionBase;
    register struct Screen *a0 __asm("a0") = screen;

    __asm __volatile ("jsr %%a6@(-0x42);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void CloseWindow(struct Window *window)
{
    register const struct IntuitionBase *a6 __asm("a6") = IntuitionBase;
    register struct Window *a0 __asm("a0") = window;

    __asm __volatile ("jsr %%a6@(-0x48);"
		      : /* no output */
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline struct Screen *OpenScreen(struct NewScreen *newScreen)
{
    register struct Screen *_res __asm("d0");
    register const struct IntuitionBase *a6 __asm("a6") = IntuitionBase;
    register struct NewScreen *a0 __asm("a0") = newScreen;

    __asm __volatile ("jsr %%a6@(-0xc6);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline struct Window *OpenWindow(struct NewWindow *newWindow)
{
    register struct Window *_res __asm("d0");
    register const struct IntuitionBase *a6 __asm("a6") = IntuitionBase;
    register struct NewWindow *a0 __asm("a0") = newWindow;

    __asm __volatile ("jsr %%a6@(-0xcc);"
		      : "=r" (_res)
		      : "r" (a6), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}
#endif /* !__ASSEMBLY__ */

#endif  /* _amigaos_h */
