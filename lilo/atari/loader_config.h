/* loader_config.h -- Compile time options for Atari boot loader
 *
 * Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: loader_config.h,v 1.1 1998-03-02 14:04:11 rnhodek Exp $
 * 
 * $Log: loader_config.h,v $
 * Revision 1.1  1998-03-02 14:04:11  rnhodek
 * New header for compile time options.
 *
 */

#ifndef _loader_config_h
#define _loader_config_h

/* Define to omit GUI code (~ 12kByte): */

/* #define NO_GUI */

/* Define to omit boot monitor (~ 10kByte): */
/* #define NO_MONITOR */

/* Define to include code to debug the ReadSector and WriteSector functions,
 * and the internal HD driver: */

/* #define DEBUG_RW_SECTORS */

/* Define to include a GEMDOS system call tracer (ca be used for debugging the
 * loader itself, or programs started by it) */

/* #define STRACE_TOS */

#endif  /* _loader_config_h */

