/*
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: loader_common.h,v 1.2 1997-09-19 09:06:48 geert Exp $
 * 
 * $Log: loader_common.h,v $
 * Revision 1.2  1997-09-19 09:06:48  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:26:57  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _loader_common_h
#define _loader_common_h

#include <config.h>

extern int Debug;
extern u_char *MapData;
extern u_long MapSize, MapOffset;
extern struct BootOptions *BootOptions;
extern struct BootRecord *BootRecords;
extern struct FileDef *Files;

extern long ReadSectors( char *buf, unsigned int device, unsigned int sector,
						 unsigned int cnt );

#endif  /* _loader_common_h */
