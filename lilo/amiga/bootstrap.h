/*
 * bootstrap.h -- Definitions for all sources of Amiga lilo
 *
 * Copyright (c) 1997 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id $
 * 
 * $Log: bootstrap.h,v $
 * Revision 1.1  1997-07-16 12:59:12  rnhodek
 * Add definitions for generic output and memory allocation
 *
 *
 * 
 */

#ifndef _bootstrap_h
#define _bootstrap_h

#define Alloc(size)		AllocVec((size),MEMF_FAST|MEMF_PUBLIC)
#define Free(p)			FreeVec(p)

#endif  /* _bootstrap_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
