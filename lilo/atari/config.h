/*
 *  Atari Linux/m68k Loader -- Configuration
 *
 *  © Copyright 1995 by Geert Uytterhoeven, Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: config.h,v 1.5 1998-03-06 09:49:07 rnhodek Exp $
 * 
 * $Log: config.h,v $
 * Revision 1.5  1998-03-06 09:49:07  rnhodek
 * New field 'modif_mask' in BootBlock.
 *
 * Revision 1.4  1998/02/26 10:16:38  rnhodek
 * New config vars WorkDir, Environ, and BootDrv (global)
 *
 * Revision 1.3  1997/09/19 09:06:56  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.2  1997/08/23 22:47:30  rnhodek
 * Added forgotten icdpart in struct BootBlock
 *
 * Revision 1.1  1997/08/12 15:27:08  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _conf_atari_h
#define _conf_atari_h

#include <sys/types.h>

    /*
     *	Boot Operating Systems
     */

#define BOS_TOS		(0)	/* TOS (start hd driver) */
#define BOS_LINUX	(1)	/* Linux/68k */
#define BOS_BOOTB	(2)	/* other (execute boot sector) */

    /*
     *	Definition of a Boot Sector
     */

struct partition {
    unsigned char	flag;		/* valid flag and boot type */
    char		id[3];		/* partition ID */
    unsigned long	start;		/* start sector */
    unsigned long	size;		/* size of partition */
};

struct BootBlock {
    short		jump;		/* jump to boot code */
    char		biosparam[34];	/* MSDOS-FS bootsector parameters */
    unsigned long	LiloID;		/* Magic for LILO */
    short		boot_device;	/* device to load the loader from */
    short		modif_mask;	/* modifiers on which Lilo skips */
#ifdef SECONDARY_ROOTSEC_NEEDED
    unsigned long	second_rootsec;	/* sector# of secondary rootsector */
#endif
    unsigned long	map_sector;	/* sector# of map sector */
    char		data[294];	/* boot loader code */
    struct partition	icdpart[8];	/* info for ICD-partitions 5..12 */
    char		unused[12];
    unsigned long	hd_size;	/* size of disk */
    struct partition	part[4];	/* partition entries */
    unsigned long	bsl_st;		/* start of bad sector list */
    unsigned long	bsl_cnt;	/* size of bad sector list */
    unsigned short	checksum;	/* checksum over root sector */
} __attribute__((__packed__));


#define MAX_TMPMNT	4
#define MAX_EXECPROG	4
#define MAX_ENVIRON	10

    /*
     *  Temporary Mount Definitions
     */

struct tmpmnt {
    int device;		/* BIOS device number */
    unsigned long start_sec;	/* abs. start sector of partition */
    unsigned int drive;			/* drive number (0=A:, 1=B:, ... */
    unsigned int rw;			/* read/write flag */
};


/* Alert stuff */
#define AN_LILO		0
enum AlertCodes {
    AG_NoMemory = 0,
    AO_LiloMap
};
void Alert( enum AlertCodes code );


#include "config_common.h"

#endif  /* _conf_atari_h */

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
