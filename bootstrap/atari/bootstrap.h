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
 * $Id: bootstrap.h,v 1.1 1997-07-15 09:45:38 rnhodek Exp $
 * 
 * $Log: bootstrap.h,v $
 * Revision 1.1  1997-07-15 09:45:38  rnhodek
 * Initial revision
 *
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

#endif  /* _bootstrap_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
