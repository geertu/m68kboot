/*
 *  Linux/m68k Loader -- Prototypes of lilo.l.c
 *
 *  © Copyright 1997 by Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: lilo.h,v 1.4 1998-02-23 10:23:22 rnhodek Exp $
 * 
 * $Log: lilo.h,v $
 * Revision 1.4  1998-02-23 10:23:22  rnhodek
 * Move #endif to end of file...
 *
 * Revision 1.3  1997/08/23 23:11:08  rnhodek
 * New parameter 'set_bootdev' to parse_device
 *
 * Revision 1.2  1997/08/23 20:36:19  rnhodek
 * Deleted obsolete prototype of CheckVectorDevice
 *
 * Revision 1.1  1997/08/12 15:27:09  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _lilo_h
#define _lilo_h

#include "lilo_common.h"
#include "config.h"

enum flopok {
	NO_FLOPPY, CUR_FLOPPY, ANY_FLOPPY
};

/***************************** Prototypes *****************************/

void parse_device( char *device, int *devnum, unsigned long *start, int
                   allow_xgm, enum flopok floppy_ok, int set_bootdev );
void CheckVectorDevice( const char *name, dev_t device, struct vecent
                        *vector );
int main( int argc, char *argv[] );

/************************* End of Prototypes **************************/

#endif  /* _lilo_h */
