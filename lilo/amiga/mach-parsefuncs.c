/*
 *  Amiga Linux/m68k Loader -- Configuration File Parser Utility Functions
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: mach-parsefuncs.c,v 1.1 1997-08-12 15:27:05 rnhodek Exp $
 * 
 * $Log: mach-parsefuncs.c,v $
 * Revision 1.1  1997-08-12 15:27:05  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#define MACH_HDROPTS				\
	  | hdropts altdev			\
	  | hdropts aux				\
	  | hdropts baud			\
	  | hdropts kickrange			\
	  | hdropts g_chipram			\
	  | hdropts g_fastram			\
	  | hdropts g_model			\
	  | hdropts g_processor			\
	  | hdropts video			\
	  | hdropts serial

#define MACH_BOOTOPTS				\
	  | bootopts type			\
	  | bootopts chipram			\
	  | bootopts fastram			\
	  | bootopts model			\
	  | bootopts processor


static const u_long *CopyLong2(u_long data1, u_long data2)
{
    u_long *p;

    if (!(p = malloc(2*sizeof(u_long))))
	Error_NoMemory();
    p[0] = data1;
    p[1] = data2;
    return(p);
}


static const u_short *CopyShort2(u_short data1, u_short data2)
{
    u_short *p;

    if (!(p = malloc(2*sizeof(u_short))))
	Error_NoMemory();
    p[0] = data1;
    p[1] = data2;
    return(p);
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
