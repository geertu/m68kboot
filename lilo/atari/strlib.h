/* strlib.h -- 
 *
 * Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: strlib.h,v 1.2 1998-04-07 09:48:17 rnhodek Exp $
 * 
 * $Log: strlib.h,v $
 * Revision 1.2  1998-04-07 09:48:17  rnhodek
 * Include stdlib.h and string.h from here.
 *
 * Revision 1.1  1998/04/07 09:43:00  rnhodek
 * *** empty log message ***
 *
 */
#ifndef _strlib_h
#define _strlib_h

/* This is a dummy on Atari, because we use normal libc string functions. Just
 * include the headers for them. */
#include <stdlib.h>
#include <string.h>

#endif  /* _strlib_h */

