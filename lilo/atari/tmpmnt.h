/*
 * tmpmnt.h -- Definitions for temporary mounts
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: tmpmnt.h,v 1.2 1998-02-26 10:34:25 rnhodek Exp $
 * 
 * $Log: tmpmnt.h,v $
 * Revision 1.2  1998-02-26 10:34:25  rnhodek
 * New function list_mounts().
 *
 * Revision 1.1  1997/08/12 15:27:13  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _tmpmnt_h
#define _tmpmnt_h


/***************************** Prototypes *****************************/

int mount( struct tmpmnt *p );
int umount( void );
int umount_drv( int drv );
void list_mounts( void );

/************************* End of Prototypes **************************/

#endif  /* _tmpmnt_h */
