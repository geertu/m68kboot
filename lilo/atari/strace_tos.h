/* strace_tos.h -- Strace-like tracing for GEMDOS system calls
 *
 * Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: strace_tos.h,v 1.2 1998-03-03 11:34:28 rnhodek Exp $
 * 
 * $Log: strace_tos.h,v $
 * Revision 1.2  1998-03-03 11:34:28  rnhodek
 * Include loader_config.h instead of loader.h.
 * Definitions should also be done if STRACE_TOS is not defined.
 *
 * Revision 1.1  1998/02/27 10:23:28  rnhodek
 * New header file for GEMDOS system call tracer (experimental).
 *
 *
 */

#ifndef _strace_tos_h
#define _strace_tos_h

#include "loader_config.h"

/* call classes to trace */
#define TR_BIOS		(1 << 0)	/* BIOS functions */
#define TR_XBIOS	(1 << 1)	/* XBIOS functions */
#define TR_CHAR		(1 << 2)	/* character device I/O */
#define TR_OPEN		(1 << 3)	/* Fopen and Fcreate */
#define TR_FILE		(1 << 4)	/* TR_OPEN plus other file-related calls */
#define TR_IO		(1 << 5)	/* file I/O */
#define TR_DIR		(1 << 6)	/* directory-related calls */
#define TR_DATE		(1 << 7)	/* date/time functions */
#define TR_PROC		(1 << 8)	/* process management */
#define TR_MEM		(1 << 9)	/* Malloc & Co. */

#define TR_ALL		(-1)		/* all calls */
#define TR_ALLGEMDOS (TR_ALL & ~(TR_BIOS|TR_XBIOS))	/* only all GEMDOS calls */

#ifdef STRACE_TOS

/***************************** Prototypes *****************************/

void strace_on( int what );
void strace_off( void );

/************************* End of Prototypes **************************/

#else /* STRACE_TOS */

#define strace_on(x)
#define strace_off()

#endif /* STRACE_TOS */
#endif  /* _strace_tos_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
