/*
 *  Amiga Linux/m68k Loader -- Reset Routine for the Reset Boot Block
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: reset.l.S,v 1.1 1997-08-12 15:27:06 rnhodek Exp $
 * 
 * $Log: reset.l.S,v $
 * Revision 1.1  1997-08-12 15:27:06  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


#include <linux/linkage.h>


    /*
     *	Exec Library Vector Offsets
     */

LVOColdReboot		= -0x2d6


.text

    /*
     *	Reset Code
     *
     *	    in:	    a6 = execbase
     *		    a chaotic pile of disoriented bits
     *
     *	    out:    a breeding ground for an altogether totally integrated
     *		    living system
     */

ENTRY(_ResetStart)
		jmp	%a6@(LVOColdReboot)
ENTRY(_ResetEnd)
