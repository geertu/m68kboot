/*
 *  Atari Linux/m68k LILO -- Configuration File Parsing
 *
 *  Copyright 1997 by Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parser.h,v 1.1 1997-08-12 15:27:12 rnhodek Exp $
 * 
 * $Log: parser.h,v $
 * Revision 1.1  1997-08-12 15:27:12  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _parser_h
#define _parser_h

#include "parser_common.h"

struct Config {
    const char *BootDevice;
    struct TagTmpMnt *Mounts;
	CONFIG_COMMON
};

#define MACH_CONFIG_INIT													\
    NULL, NULL,																\
    { NULL, NULL, NULL, NULL, NULL, NULL, { NULL, }, { NULL, },				\
	  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },	\
    NULL, NULL


#endif  /* _parser_h */
