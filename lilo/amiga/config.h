/*
 *  Amiga Linux/m68k Loader -- Configuration
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: config.h,v 1.2 1997-08-12 21:51:05 rnhodek Exp $
 * 
 * $Log: config.h,v $
 * Revision 1.2  1997-08-12 21:51:05  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _conf_amiga_h
#define _conf_amiga_h

#include "linuxboot.h"
#include "config_common.h"

#define ID_BOOT_DISK		(0x424F4F55)	/* `BOOU' */
#define ID_OFS_DISK		(0x444F5300)	/* `DOS\0' */
#define ID_FFS_DISK		(0x444F5301)	/* `DOS\1' */
#define ID_OFS_INTL_DISK	(0x444F5302)	/* `DOS\2' */
#define ID_FFS_INTL_DISK	(0x444F5303)	/* `DOS\3' */
#define ID_OFS_DC_DISK		(0x444F5304)	/* `DOS\4' */
#define ID_FFS_DC_DISK		(0x444F5305)	/* `DOS\5' */
#define ID_MUFS_DISK		(0x6d754653)	/* `muFS' */
#define ID_MU_OFS_DISK		(0x6d754600)	/* `muF\0' */
#define ID_MU_FFS_DISK		(0x6d754601)	/* `muF\1' */
#define ID_MU_OFS_INTL_DISK	(0x6d754602)	/* `muF\2' */
#define ID_MU_FFS_INTL_DISK	(0x6d754603)	/* `muF\3' */
#define ID_MU_OFS_DC_DISK	(0x6d754604)	/* `muF\4' */
#define ID_MU_FFS_DC_DISK	(0x6d754605)	/* `muF\5' */


struct BootBlock {
    u_long ID;
    u_long Checksum;
    u_long DosBlock;
    u_long Data[252];
    u_long LiloID;
};

    /*
     *	Boot Operating Systems
     */

#define BOS_AMIGA		(0)	/* AmigaOS */
#define BOS_LINUX		(1)	/* Linux/m68k */
#define BOS_NETBSD		(2)	/* Future Expansion :-) */

#endif  /* _conf_amiga_h */

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
