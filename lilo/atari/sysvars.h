/*
 * sysvars.h -- Definitions for some TOS system variables
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: sysvars.h,v 1.2 1998-02-24 11:21:36 rnhodek Exp $
 * 
 * $Log: sysvars.h,v $
 * Revision 1.2  1998-02-24 11:21:36  rnhodek
 * _bootdev is a word variable, not long.
 *
 * Revision 1.1  1997/08/12 15:27:13  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _sysvars_h
#define _sysvars_h

#define	_bootdev	(*((unsigned short *)0x446))
#define hdv_getbpb	(*((unsigned long *)0x472))
#define hdv_rw		(*((unsigned long *)0x476))
#define hdv_mediach	(*((unsigned long *)0x47e))

#define	HZ			200
#define	_hz_200		(*(volatile unsigned long *)0x4ba)

#define _drvbits	(*((unsigned long *)0x4c2))
#define _dskbuf		(*((char **)0x4c6))

#define _p_cookies	((unsigned long **)0x5a0)

#endif  /* _sysvars_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
