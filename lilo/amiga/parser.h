/*
 *  Amiga Linux/m68k Loader -- Configuration File Parsing
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parser.h,v 1.1 1997-08-12 15:27:05 rnhodek Exp $
 * 
 * $Log: parser.h,v $
 * Revision 1.1  1997-08-12 15:27:05  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _parser_h
#define _parser_h

#include <asm/amigahw.h>
#include "parser_common.h"

    /*
     *	Alternate Boot Partition
     *
     *	Warning: this definition is hardcoded in bootcode.s too!
     */

#define MAXALTDEVSIZE	(32)


    /*
     *	Configuration File Parsing
     */

struct Config {
    const char *BootDevice;
    const char *AltDeviceName;
    const u_long *AltDeviceUnit;
	CONFIG_COMMON
};

#define MACH_CONFIG_INIT								\
    NULL, NULL, NULL,									\
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },	\
    NULL, NULL

#endif  /* _parser_h */
