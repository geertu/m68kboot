/*
 *  Linux/m68k Loader -- Version Control
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: lilo_common.h,v 1.3 1998-02-23 10:15:21 rnhodek Exp $
 * 
 * $Log: lilo_common.h,v $
 * Revision 1.3  1998-02-23 10:15:21  rnhodek
 * Update copyright years in banner message.
 *
 * Revision 1.2  1997/09/19 09:06:47  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:26:56  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _lilo_common_h
#define _lilo_common_h

#include "version.h"

#define LILO_VERSION "Linux/m68k Loader version " VERSION WITH_BOOTP "\n" \
					 "(C) Copyright 1995-1998 by Geert Uytterhoeven, Roman Hodek\n"

extern const char LiloVersion[];

#endif  /* _lilo_common_h */
