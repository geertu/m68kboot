/*
 * linuxboot.c -- Do actual booting of Linux kernel
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
 * $Id: inline-funcs.h,v 1.1 1997-07-15 09:45:38 rnhodek Exp $
 * 
 * $Log: inline-funcs.h,v $
 * Revision 1.1  1997-07-15 09:45:38  rnhodek
 * Initial revision
 *
 * 
 */

static __inline__ int fpu_idle_frame_size( void);
static __inline__ void change_stack( u_long *stackp);
static __inline__ void disable_interrupts( void);
static __inline__ void disable_cache( void);
static __inline__ void disable_mmu( void);
static __inline__ void jump_to_mover( void *kernel_start, void *mem_start,
                                      void *ramdisk_end, void *mem_end, int
                                      kernel_size, int ramdisk_size, void
                                      *mover_addr) __attribute__((noreturn));

/* some inline functions */

static __inline__ int fpu_idle_frame_size (void)
{
    char fpu_frame[216];

    __asm__ __volatile__ ("fnop"::);
    __asm__ __volatile__ ("fsave %0@" : : "a" (fpu_frame));
    return fpu_frame[1];
}

static __inline__ void change_stack (u_long *stackp)
{
    __asm__ volatile ("movel %0,sp\n\t" :: "g" (stackp) : "sp");
}

static __inline__ void disable_interrupts (void)
{
    __asm__ volatile ("orw #0x700,sr":);
}

extern struct atari_bootinfo bi;
static __inline__ void disable_cache (void)
{
    __asm__ volatile ("movec %0,cacr" :: "d" (0));
    if (bi.cputype & CPU_68060) {
	/* '060: clear branch cache after disabling it;
	 * disable superscalar operation (and enable FPU) */
	__asm__ volatile ("movec %0,cacr" :: "d" (0x00400000));
	__asm__ volatile ("moveq #0,d0;"
			  ".long 0x4e7b0808"	/* movec d0,pcr */
			  : /* no outputs */
			  : /* no inputs */
			  : "d0");
    }
}

static __inline__ void disable_mmu (void)
{
    if (bi.cputype & (CPU_68040|CPU_68060)) {
	__asm__ volatile ("moveq #0,d0;"
			  ".long 0x4e7b0003;"	/* movec d0,tc */
			  ".long 0x4e7b0004;"	/* movec d0,itt0 */
			  ".long 0x4e7b0005;"	/* movec d0,itt1 */
			  ".long 0x4e7b0006;"	/* movec d0,dtt0 */
			  ".long 0x4e7b0007"	/* movec d0,dtt1 */
			  : /* no outputs */
			  : /* no inputs */
			  : "d0");
    }
    else {
	__asm__ volatile ("subl  #4,sp\n\t"
			  "pmove tc,sp@\n\t"
			  "bclr  #7,sp@\n\t"
			  "pmove sp@,tc\n\t"
			  "addl  #4,sp");
	if (bi.cputype & CPU_68030) {
	    __asm__ volatile ("clrl sp@-\n\t"
			      ".long 0xf0170800\n\t" /* pmove sp@,tt0 */
			      ".long 0xf0170c00\n\t" /* pmove sp@,tt1 */
			      "addl  #4,sp\n");
	}
    }
}

static __inline__ void jump_to_mover (void *kernel_start, void *mem_start,
				      void *ramdisk_end, void *mem_end,
				      int kernel_size, int ramdisk_size,
				      void *mover_addr)
{
    asm volatile ("movel %0,a0\n\t"
		  "movel %1,a1\n\t"
		  "movel %2,a2\n\t"
		  "movel %3,a3\n\t"
		  "movel %4,d0\n\t"
		  "movel %5,d1\n\t"
		  "jmp   %6@\n"
		  : /* no outputs */
		  : "g" (kernel_start), "g" (mem_start),
		    "g" (ramdisk_end), "g" (mem_end),
		    "g" (kernel_size), "g" (ramdisk_size),
		    "a" (mover_addr)
		  : "a0", "a1", "a2", "a3", "d0", "d1");

	/* Avoid warning that function may return */
	for (;;) ;
}
