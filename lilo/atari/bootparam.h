/* bootparm.h -- Definition for layout of boot.b and map files for Atari LILO
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: bootparam.h,v 1.1 1997-08-12 15:27:08 rnhodek Exp $
 * 
 * $Log: bootparam.h,v $
 * Revision 1.1  1997-08-12 15:27:08  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _bootparm_h
#define _bootparm_h

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
#define	BP_CMDLIN		128

/*
 * layout of loader
 * ----------------
 */

/* unions are used to force block sizes */

#define	N_MAPENTRIES	85
#define	MAX_TMPMOUNT	4

#ifndef __ASSEMBLY__

typedef struct {
	/* root sector */
	union {
		char length[512];
		struct {
			u_short	branch;
			char	biosparam[34];
			u_long	lilo_magic;
			short	bootdev;
#ifdef SECONDARY_ROOTSEC_NEEDED
			u_long	next_sector;
#endif
			u_long	map_sector;
		} rootsec_s __attribute((packed));
	} rootsec_u;
#ifdef SECONDARY_ROOTSEC_NEEDED
	/* Space for a second root sector, to extend the first. This is currently
	 * not needed, all code fits into the master root sector. */
	char secondary_rootsec[512];
#endif /* SECONDARY_ROOTSEC_NEEDED */
	/* map sector */
	union {
		char length[512];
		struct maprec map_entries[N_MAPENTRIES];
	} mapblock_u;
	/* start of TOS program */
	struct {
		TOS_PRG_HDR header;
		u_short	branch;
		u_short	timeout;		/* 5 msec delay until input time-out,
								 * 0xffff: never */
		u_short	delay;			/* delay: wait that many 5 msec units. */
		u_short	port;			/* serial port. 0 = none, 1 = Modem1, etc. */
		u_long	ser_param;		/* RS-232 parameters (low word: speed, high
								 * word: ucr for Rsconf()) */
		SECTOR_ADDR descr[3];
		u_short	prompt;			/* 0 = only on demand, =! 0 = always */
		u_short	msg_len;		/* 0 if none */
		SECTOR_ADDR msg;		/* message sector */
		u_short tmpmount[MAX_TMPMOUNT];	/* temp mount descriptors */
		
		
	} tosprg  __attribute((packed));
} BOOT_SECTOR  __attribute((packed));

#define	lilo_magic		rootsec_u.rootsec_s.lilo_magic
#define	bootdev			rootsec_u.rootsec_s.bootdev
#define	next_sector		rootsec_u.rootsec_s.next_sector
#define	map_sector		rootsec_u.rootsec_s.map_sector

#define map_entries		mapblock_u.map_entries

#endif /* __ASSEMBLY__ */

#endif  /* _bootparm_h */
