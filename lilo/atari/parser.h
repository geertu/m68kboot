/*
 *  Atari Linux/m68k LILO -- Configuration File Parsing
 *
 *  Copyright 1997 by Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parser.h,v 1.5 1998-03-10 10:31:53 rnhodek Exp $
 * 
 * $Log: parser.h,v $
 * Revision 1.5  1998-03-10 10:31:53  rnhodek
 * New option "message".
 *
 * Revision 1.4  1998/03/06 09:50:39  rnhodek
 * New option skip-on-keys.
 *
 * Revision 1.3  1998/03/04 09:19:03  rnhodek
 * New config var array ProgCache[] as option to 'exec'.
 * WorkDir and ProgCache also for record-specific exec's.
 *
 * Revision 1.2  1998/02/26 10:27:50  rnhodek
 * New config vars WorkDir, Environ, and BootDrv (global)
 *
 * Revision 1.1  1997/08/12 15:27:12  rnhodek
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
    const u_long *SkipOnKeys;
    struct TagTmpMnt *Mounts;
	CONFIG_COMMON
};

#define MACH_CONFIG_INIT													\
    NULL, NULL,	NULL,														\
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, { NULL, }, { NULL, },		\
	  { NULL, }, { NULL, },  NULL, NULL, NULL, NULL, NULL, NULL, NULL,		\
	  NULL, NULL, NULL, NULL, NULL, { NULL } },								\
    NULL, NULL


#endif  /* _parser_h */
