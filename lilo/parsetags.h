/*
 *  parsetags.h -- definitions for parsetags.c
 *
 *  © Copyright 1995 by Geert Uytterhoeven, Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parsetags.h,v 1.1 1997-08-12 15:26:58 rnhodek Exp $
 * 
 * $Log: parsetags.h,v $
 * Revision 1.1  1997-08-12 15:26:58  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _parsetags_h
#define _parsetags_h

#include <sys/types.h>

/***************************** Prototypes *****************************/

void ParseTags( void );
void FreeMapData( void);
const struct BootRecord *FindBootRecord( const char *name);
const struct vecent *FindVector( const char *path);

/************************* End of Prototypes **************************/

#endif  /* _parsetags_h */

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
