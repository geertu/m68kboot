/*
 *  Amiga Linux/m68k Loader -- Boot Code Library
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: bootlib.h,v 1.1 1997-08-12 15:27:02 rnhodek Exp $
 * 
 * $Log: bootlib.h,v $
 * Revision 1.1  1997-08-12 15:27:02  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


    /*
     *	Function Table (Passed at Startup)
     */

#ifndef __ASSEMBLY__

#include "config.h"

struct LiloTable {
    void *lilo_open;
    void *lilo_close;
    void *lilo_read;
    void *lilo_seek;
    void *lilo_load;
    void *lilo_unload;
    void *lilo_exec;
};

extern const struct LiloTable *LiloTable;


    /*
     *	Open a File
     */

static __inline void Lilo_Open(const struct vecent *vector)
{
    register void **a1 __asm("a1") = LiloTable->lilo_open;
    register const struct vecent *a0 __asm("a0") = vector;

    __asm __volatile ("jsr a1@"
		      : /* no output */
		      : "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}


    /*
     *	Close the File
     */

static __inline void Lilo_Close(void)
{
    register void **a1 __asm("a1") = LiloTable->lilo_close;

    __asm __volatile ("jsr a1@"
		      : /* no output */
		      : "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}


    /*
     *	Read a Block from the File
     */

static __inline void Lilo_Read(void *dest, u_long length)
{
    register void **a1 __asm("a1") = LiloTable->lilo_read;
    register void *a0 __asm("a0") = dest;
    register u_long d0 __asm("d0") = length;

    __asm __volatile ("jsr a1@"
		      : /* no output */
		      : "r" (a0), "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
}


    /*
     *	Absolute Seek
     */

static __inline void Lilo_Seek(u_long offset)
{
    register void **a1 __asm("a1") = LiloTable->lilo_seek;
    register u_long d0 __asm("d0") = offset;

    __asm __volatile ("jsr a1@"
		      : /* no output */
		      : "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
}


    /*
     *	Load a File
     */

static __inline u_long *Lilo_Load(const u_long *vector)
{
    register u_long *_res __asm("d0");
    register void **a1 __asm("a1") = LiloTable->lilo_load;
    register const u_long *a0 __asm("a0") = vector;

    __asm __volatile ("jsr a1@"
		      : "=r" (_res)
		      : "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


    /*
     *	Unload a File
     */

static __inline void Lilo_UnLoad(u_long *data)
{
    register void **a1 __asm("a1") = LiloTable->lilo_unload;
    register u_long *a0 __asm("a0") = data;

    __asm __volatile ("jsr a1@"
		      : /* no output */
		      : "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}


    /*
     *	Execute a Loaded File
     */

static __inline long Lilo_Exec(u_long *data, long arg)
{
    register long _res __asm("d0");
    register void **a1 __asm("a1") = LiloTable->lilo_exec;
    register u_long *a0 __asm("a0") = data;
    register long d0 __asm("d0") = arg;

    __asm __volatile ("jsr a1@"
		      : "=r" (_res)
		      : "r" (a0), "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

#else /* __ASSEMBLY__ */

Lilo_Open	= 0
Lilo_Close	= 4
Lilo_Read	= 8
Lilo_Seek	= 12
Lilo_Load	= 16
Lilo_Unload	= 20
Lilo_Exec	= 24

#endif /* __ASSEMBLY__ */
