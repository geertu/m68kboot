/*
 *  Linux/m68k Loader -- General Utilities
 *
 *  © Copyright 1995-97 by Geert Uytterhoeven, Roman Hodek
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: lilo_util.h,v 1.1 1997-08-12 15:26:57 rnhodek Exp $
 * 
 * $Log: lilo_util.h,v $
 * Revision 1.1  1997-08-12 15:26:57  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _lilo_util_h
#define _lilo_util_h

#include "config.h"

extern const char *ProgramName;
extern int Verbose;
extern int DoBackup;
extern int Install;
extern int Uninstall;
extern const char *SaveBootBlock;
extern const char *RestoreBootBlock;
extern const char *Device;
extern const char *Root;
extern const char *ConfigFile;
extern const char *MapFile;
extern const char *LoaderTemplate;
extern const char *LoaderFile;
extern const char *BackupFile;
extern dev_t BootDevice;
extern struct BootBlock BootBlock;
extern u_long HoleSector;
extern u_long MaxHoleSectors;

extern void CheckVectorDevice( struct vecent *vector, dev_t device );

/***************************** Prototypes *****************************/

void Die( const char *fmt, ...);
void Error_NoMemory( void);
void Error_Open( const char *name);
void Error_OpenDir( const char *name);
void Error_Read( const char *name);
void Error_Write( const char *name);
void Error_Seek( const char *name);
void Error_Stat( const char *name);
void Error_Ioctl( const char *name, const char *ioctl);
int ReadFile( const char *name, void **data);
void WriteFile( const char *name, const void *data, int size);
void ReadBootBlock( const char *name, u_long sector);
void WriteBootBlock( const char *name, u_long sector);
void BackupBootBlock( void);
void ReadConfigFile( void);
int EqualStrings( const char *s1, const char *s2);
void FixSpecialChars( char *s);
int AddBootRecord( const struct BootRecord *record);
void AddFileDef( const char *path);
const char *CreateFileName( const char *name);
const char *CreateBackupFileName( void);
const struct vecent *CreateVector( const char *name, int *numblocks );
void FindDevice( dev_t device, char *devname );
void FindFileDevice( const char *name, char *devname, dev_t *device );
void GeometryFile( const char *name, dev_t *device, u_long *start);
void GeometryDevice( const char *devname, u_long *start);

/************************* End of Prototypes **************************/

#undef MAJOR
#define	MAJOR(dev)	(((unsigned)(dev) >> 8) & 0xff)
#undef MINOR
#define	MINOR(dev)	((unsigned)(dev) & 0xff)

#define HARD_SECTOR_SIZE	512

#endif  /* _lilo_util_h */
