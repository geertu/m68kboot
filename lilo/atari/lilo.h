/*
 *  Linux/m68k Loader -- Prototypes of lilo.l.c
 *
 *  © Copyright 1997 by Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: lilo.h,v 1.1 1997-08-12 15:27:09 rnhodek Exp $
 * 
 * $Log: lilo.h,v $
 * Revision 1.1  1997-08-12 15:27:09  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _lilo_h
#define _lilo_h

#include "lilo_common.h"
#include "config.h"

#endif  /* _lilo_h */

enum flopok {
	NO_FLOPPY, CUR_FLOPPY, ANY_FLOPPY
};

/***************************** Prototypes *****************************/

void parse_device( const char *device, int *devnum, unsigned long *start,
                   int allow_xgm, enum flopok floppy_ok );
void CheckVectorDevice( struct vecent *vector, dev_t device );
int main( int argc, char *argv[] );

/************************* End of Prototypes **************************/

