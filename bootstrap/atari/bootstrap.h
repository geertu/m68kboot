/*
 * bootstrap.h -- Definitions for all sources of Atari bootstrap
 *
 * Copyright (c) 1993-97 by
 *   Arjan Knor
 *   Robert de Vries
 *   Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *   Andreas Schwab <schwab@issan.informatik.uni-dortmund.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: bootstrap.h,v 1.3 1997-07-16 15:06:21 rnhodek Exp $
 * 
 * $Log: bootstrap.h,v $
 * Revision 1.3  1997-07-16 15:06:21  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.2  1997/07/16 12:59:13  rnhodek
 * Add definitions for generic output and memory allocation
 *
 * Revision 1.1.1.1  1997/07/15 09:45:38  rnhodek
 * Import sources into CVS
 * 
 */

#ifndef _bootstrap_h
#define _bootstrap_h

#include <stdio.h>
#include <osbind.h>

extern unsigned long userstk;

/* ++andreas: this must be inline due to Super */
extern __inline__ void boot_exit (int) __attribute__ ((noreturn));
extern __inline__ void boot_exit(int status)
{
    /* first go back to user mode */
    (void)Super(userstk);
    getchar();
    exit(status);
}

#define Puts			puts
#define	GetChar			getchar
#define	PutChar			putchar
#define	Printf			printf
#define Alloc			malloc
#define Free			free
#define ReAlloc(p,o,n)	realloc((p),(n))

#endif  /* _bootstrap_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
