/* loader.h -- Common definitions for Atari boot loader
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: loader.h,v 1.2 1997-09-19 09:06:58 geert Exp $
 * 
 * $Log: loader.h,v $
 * Revision 1.2  1997-09-19 09:06:58  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:10  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */
#ifndef _loader_h
#define _loader_h

#include "loader_common.h"

extern unsigned int SerialPort;
extern unsigned int AutoBoot;
extern const char *Prompt;
extern unsigned int NoGUI;
extern const struct BootRecord *dflt_os;
extern int CurrentFloppy;
extern struct TagTmpMnt *MountPointList;
extern struct tmpmnt *MountPoints;

/***************************** Prototypes *****************************/

int is_available( const struct BootRecord *rec );
void boot_tos( const struct BootRecord *rec );
void boot_linux( const struct BootRecord *rec, const char *cmdline );
void boot_bootsector( const struct BootRecord *rec );
int exec_tos_program( const char *prog );
const char *tos_perror( long err );
void MachInitDebug( void );
void Alert( enum AlertCodes code );
long WriteSectors( char *buf, int device, unsigned int sector, unsigned int cnt );

/************************* End of Prototypes **************************/


#endif  /* _loader_h */

