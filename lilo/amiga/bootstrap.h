/*
 * bootstrap.h -- Definitions for all sources of Amiga lilo
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: bootstrap.h,v 1.4 1997-09-19 09:06:51 geert Exp $
 * 
 * $Log: bootstrap.h,v $
 * Revision 1.4  1997-09-19 09:06:51  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.3  1997/08/12 15:18:25  rnhodek
 * Moved AmigaOS inline funcs to extr header inline-funcs.h; the functions
 * can't be compiled under Linux
 *
 * Revision 1.2  1997/07/16 15:06:24  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.1  1997/07/16 12:59:12  rnhodek
 * Add definitions for generic output and memory allocation
 * 
 */

#ifndef _bootstrap_h
#define _bootstrap_h

#include "linuxboot.h"
#include "inline-funcs.h"

#define Alloc(size)		AllocVec((size),MEMF_FAST|MEMF_PUBLIC)
#define Free(p)			FreeVec(p)
#define	ReAlloc(ptr,oldsize,newsize)				\
	({												\
		void *__ptr = (ptr);						\
		void *__newptr;								\
		size_t __oldsize = (oldsize);				\
		size_t __newsize = (newsize);				\
													\
		if ((__newptr = Alloc( __newsize ))) {		\
			memcpy( __newptr, __ptr, __oldsize );	\
			Free( __ptr );							\
		}											\
		__newptr;									\
	})

#endif  /* _bootstrap_h */

extern void Puts(const char *str);
extern void Printf(const char *fmt, ...) __attribute__
    ((format (printf, 1, 2)));
extern int GetChar(void);
extern void PutChar(int c);

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
