/*
 * bootstrap.h -- Definitions for all sources of Atari lilo
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: bootstrap.h,v 1.4 1998-02-24 11:18:50 rnhodek Exp $
 * 
 * $Log: bootstrap.h,v $
 * Revision 1.4  1998-02-24 11:18:50  rnhodek
 * Define Printf (used in common sources) to be cprintf instead of simple printf.
 * Need menu.h for this, too.
 *
 * Revision 1.3  1997/07/30 21:42:51  rnhodek
 * Fix defition of Puts; make boot_exit a normal function
 *
 * Revision 1.2  1997/07/16 15:06:25  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.1  1997/07/16 12:59:13  rnhodek
 * Add definitions for generic output and memory allocation
 *
 *
 * 
 */

#ifndef _bootstrap_h
#define _bootstrap_h

#include <menu.h>

#define	boot_exit		exit

#define Puts(str)		fputs( (str), stdout )
#define	GetChar			getchar
#define	PutChar			putchar
#define	Printf			cprintf
#define Alloc			malloc
#define Free			free
#define ReAlloc(p,o,n)	realloc((p),(n))

#endif  /* _bootstrap_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
