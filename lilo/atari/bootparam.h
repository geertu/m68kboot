/* bootparm.h -- Definition for layout of boot.b and map files for Atari LILO
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: bootparam.h,v 1.4 1998-02-23 10:17:39 rnhodek Exp $
 * 
 * $Log: bootparam.h,v $
 * Revision 1.4  1998-02-23 10:17:39  rnhodek
 * Added definition BP_DEVX (devices for standard handles)
 *
 * Revision 1.3  1998/02/19 20:40:14  rnhodek
 * Make things compile again
 *
 * Revision 1.2  1997/08/23 22:43:44  rnhodek
 * Removed leftovers from ancient versions
 *
 * Revision 1.1  1997/08/12 15:27:08  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 * 
 */

#ifndef _bootparm_h
#define _bootparm_h

/* number of struct vecent's that fit into a sector: int(512/6) */
#define N_MAPENTRIES	85

#ifndef __ASSEMBLY__
#include <sys/types.h>
#include "lilo.h"
#endif /* __ASSEMBLY__ */

/*
 * definitions for TOS program header structure
 * --------------------------------------------
 */

#ifndef __ASSEMBLY__
/* structure definition */
typedef struct {
	u_short	magic;
	u_long	tlen;
	u_long	dlen;
	u_long	blen;
	u_long	slen;
	u_long	_rsvd;
	u_long	flags;
	u_short	absflag;
} TOS_PRG_HDR;
#endif /* __ASSEMBLY__ */

/* size */
#define TOSPRGHDR_SIZE	28

/* offsets */
#define	PRGHDR_MAGIC	0		/* magic: 0x601a */
#define	PRGHDR_TLEN		2		/* length of text segment */
#define	PRGHDR_DLEN		6		/* length of data segment */
#define	PRGHDR_BLEN		10		/* length of BSS segment */
#define	PRGHDR_SLEN		14		/* length of symbol table */
#define	PRGHDR_RSVD		18		/* reserved */
#define	PRGHDR_FLAGS	22		/* flags */
#define	PRGHDR_ABS		26		/* absolute code? */


/*
 * definitions for TOS base page structure
 * ---------------------------------------
 */

#define	BP_LOWTPA		0
#define	BP_HITPA		4
#define	BP_TBASE		8
#define	BP_TLEN			12
#define	BP_DBASE		16
#define	BP_DLEN			20
#define	BP_BBASE		24
#define	BP_BLEN			28
#define	BP_DTA			32
#define	BP_PARENT		36
#define	BP_FLAGS		40
#define	BP_ENV			44
#define	BP_DEVX			48
#define	BP_CMDLIN		128

#endif  /* _bootparm_h */
