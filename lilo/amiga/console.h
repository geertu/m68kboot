/*
 *  Amiga Linux/m68k Loader -- Console Definitions
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: console.h,v 1.1 1997-08-12 15:27:03 rnhodek Exp $
 * 
 * $Log: console.h,v $
 * Revision 1.1  1997-08-12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


    /*
     *  Console Functions
     */

struct Console {
    int (*Interactive)(void);
    void (*Open)(void);
    void (*Close)(void);
    void (*Puts)(const char *s);
    void (*PutChar)(char c);
    void (*PreGetChar)(void);
    int (*GetChar)(void);
    void (*PostGetChar)(void);
    int Opened;
};


    /*
     *  Video Consoles
     */

extern struct Console *Probe_Intuition(const char *args);
#if 0
extern struct Console *Probe_VGA(const char *args);
extern struct Console *Probe_Cyber(const char *args);
extern struct Console *Probe_Retz3(const char *args);
#endif


    /*
     *  Serial Consoles
     */

extern struct Console *Probe_Paula(const char *args);
#if 0
extern struct Console *Probe_IOExt(const char *args);
extern struct Console *Probe_MFC(const char *args);
#endif
