/* strace_tos.c -- Strace-like tracing for GEMDOS system calls
 *
 * Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: strace_tos.c,v 1.1 1998-02-27 10:25:25 rnhodek Exp $
 * 
 * $Log: strace_tos.c,v $
 * Revision 1.1  1998-02-27 10:25:25  rnhodek
 * New: GEMDOS system call tracer (experimental, for debugging).
 *
 *
 */

#include "loader.h"
#ifdef STRACE_TOS

#include <stdio.h>
#include <string.h>
#include <osbind.h>
#include "strace_tos.h"

#define vec_trap1	(*((unsigned long *)0x84))
#define vec_trap13	(*((unsigned long *)0xb4))
#define vec_trap14	(*((unsigned long *)0xb8))

static int VectorsInstalled = 0;
static int TraceWhat = 0;
static int InTrace = 0;
static char PrtBuf[256];

typedef struct table_entry {
	int  num;
	int  flags;
	char *args;
	char *format;
} TABENT;

static TABENT gemdos_calls[] = {
	{  0, TR_PROC,		"",			"Pterm0()" },
	{  1, TR_CHAR,		"",			"Cconin()" },
	{  2, TR_CHAR,		"w",		"Cconout(%02x)" },
	{  3, TR_CHAR,		"", 		"Cauxin()" },
	{  4, TR_CHAR,		"w",		"Cauxout(%02x)" },
	{  5, TR_CHAR,		"w",		"Cprnout(%02x)" },
	{  6, TR_CHAR,		"w",		"Crawio(%02x)" },
	{  7, TR_CHAR,		"",			"Crawcin()" },
	{  8, TR_CHAR,		"",			"Cnecin()" },
	{  9, TR_CHAR,		"l",		"Cconws(\"%s\")" },
	{ 10, TR_CHAR,		"l",		"Conrs(%08lx)" },
	{ 11, TR_CHAR,		"",			"Cconis()" },
	{ 14, TR_DIR,		"w",		"Dsetdrv(%d)" },
	{ 16, TR_CHAR,		"",			"Cconos()" },
	{ 17, TR_CHAR,		"",			"Cprnos()" },
	{ 18, TR_CHAR,		"",			"Cauxis()" },
	{ 19, TR_CHAR,		"",			"Cauxos()" },
	{ 20, TR_MEM,		"ll",		"Maddalt(%08lx,%08lx)" },
	{ 25, TR_DIR,		"",			"Dgetdrv()" },
	{ 26, TR_FILE,		"l",		"Fsetdta(%08lx)" },
	{ 32, TR_PROC,		"l",		"Super(%08lx)" },
	{ 42, TR_DATE,		"",			"Tgetdate()" },
	{ 43, TR_DATE,		"w",		"Tsetdate(%04x)" },
	{ 44, TR_DATE,		"",			"Tgettime()" },
	{ 45, TR_DATE,		"w",		"Tsettime(%04x)" },
	{ 47, TR_FILE,		"",			"Fgetdta()" },
	{ 48, TR_PROC,		"",			"Sversion()" },
	{ 49, TR_PROC,		"lw",		"Ptermres(%d,%d)" },
	{ 54, TR_DIR,		"lw",		"Dfree(%08lx,%d)" },
	{ 57, TR_DIR,		"l",		"Dcreate(\"%s\")" },
	{ 58, TR_DIR,		"l",		"Ddelete(\"%s\")" },
	{ 59, TR_DIR,		"l",		"Dsetpath(\"%s\")" },
	{ 60, TR_FILE|TR_OPEN, "lw",	"Fcreate(\"%s\",%02x)" },
	{ 61, TR_FILE|TR_OPEN, "lw",	"Fopen(\"%s\",%d)" },
	{ 62, TR_FILE,		"w",		"Fclose(%d)" },
	{ 63, TR_IO,		"wll",		"Fread(%d,%d,%08lx)" },
	{ 64, TR_IO,		"wll",		"Fwrite(%d,%d,%08lx)" },
	{ 65, TR_FILE,		"l",		"Fdelete(\"%s\")" },
	{ 66, TR_FILE,		"lww",		"Fseek(%d,%d,%d)" },
	{ 67, TR_FILE,		"lww",		"Fattrib(\"%s\",%d,%02x)" },
	{ 68, TR_MEM,		"lw",		"Mxalloc(%d,%d)" },
	{ 69, TR_FILE,		"w",		"Fdup(%d)" },
	{ 70, TR_FILE,		"ww",		"Fforce(%d,%d)" },
	{ 71, TR_DIR,		"lw",		"Dgetpath(%08lx,%d)" },
	{ 72, TR_MEM,		"l",		"Malloc(%d)" },
	{ 73, TR_MEM,		"l",		"Mfree(%08lx)" },
	{ 74, TR_MEM,		"wll",		"Mshrink(%d,%08lx,%d)" },
	{ 75, TR_PROC,		"wlll",		"Pexec(%d,%08lx,%08lx,%08lx)" },
	{ 76, TR_PROC,		"w",		"Pterm(%d)" },
	{ 78, TR_FILE,		"lw",		"Fsfirst(\"%s\",%02x)" },
	{ 79, TR_FILE,		"",			"Fsnext()" },
	{ 86, TR_FILE,		"wll",		"Frename(%d,\"%s\",\"%s\")" },
	{ 87, TR_FILE,		"lww",		"Fdattime(%08lx,%d,%d)" },
	{ -1, 0,			"",			"" }
};

static TABENT bios_calls[] = {
	{  1, TR_BIOS,		"w",		"Bconstat(%d)" },
	{  2, TR_BIOS,		"w",		"Bconin(%d)" },
	{  3, TR_BIOS,		"ww",		"Bconout(%d,%02x)" },
	{  4, TR_BIOS,		"wlwww",	"Rwabs(%d,%08lx,%u,%u,%d)" },
	{  5, TR_BIOS,		"wl",		"Setexc(%d,%08lx)" },
	{  6, TR_BIOS,		"",			"Tickcal()" },
	{  7, TR_BIOS,		"w",		"Getbpb(%d)" },
	{  8, TR_BIOS,		"w",		"Bcostat(%d)" },
	{  9, TR_BIOS,		"w",		"Mediach(%d)" },
	{ 10, TR_BIOS,		"",			"Drvmap()" },
	{ 11, TR_BIOS,		"w",		"Kbshift(%04x)" },
	{ -1, 0,			"",			"" }
};

static TABENT xbios_calls[] = {
	{ -1, 0,			"",			"" }
};

typedef struct {
	int trapno;
	TABENT *table;
} TABIDX;

static TABIDX table_index[] = {
	{ 1,  gemdos_calls },
	{ 13, bios_calls },
	{ 14, xbios_calls }
};


/***************************** Prototypes *****************************/

static void install_vectors( void );
static void uninstall_vectors( void );
static int remove_vector( unsigned long *vec, const char *vecname );
static void do_trace( int trapno, char *args )
	__attribute__ ((unused));
static void do_retval( unsigned long val )
	__attribute__ ((unused));
static void bputs( const char *p );

/************************* End of Prototypes **************************/

/* symbols defined in assembler code */
extern long my_trap1();
extern long my_trap13();
extern long my_trap14();
extern unsigned long old_trap1, old_trap13, old_trap14;

void strace_on( int what )
{
	install_vectors();
	TraceWhat = what;
}


void strace_off( void )
{
	uninstall_vectors();
}


static void install_vectors( void )
{
	if (VectorsInstalled)
		return;
	old_trap1  = vec_trap1;
	old_trap13 = vec_trap13;
	old_trap14 = vec_trap14;
	vec_trap1  = (unsigned long)my_trap1;
/*	vec_trap13 = (unsigned long)my_trap13; */
	vec_trap14 = (unsigned long)my_trap14;
	VectorsInstalled = 1;
}


static void uninstall_vectors( void )
{
	if (!VectorsInstalled)
		return;
	remove_vector( &vec_trap1,  "TRAP #1 vector" );
/*	remove_vector( &vec_trap13, "TRAP #13 vector" ); */
	remove_vector( &vec_trap14, "TRAP #14 vector" );
	VectorsInstalled = 0;
}


typedef struct {
	unsigned long magic;
	unsigned long id;
	unsigned long oldvec;
} XBRA;

static int remove_vector( unsigned long *vec, const char *vecname )
{
	XBRA *x;

	for( ; vec; vec = &x->oldvec ) {
		x = (XBRA *)(*vec) - 1;
		if (x->magic != *(unsigned long *)"XBRA") {
			printf( "Can't remove me from %s vector chain (XBRA violation)\n",
					vecname );
			return( 0 );
		}
		if (x->id == *(unsigned long *)"LILO") {
			*vec = x->oldvec;
			return( 1 );
		}
	}
	printf( "Can't find id \"LILO\" in XBRA chain of %s\n", vecname );
	return( 0 );
}

__asm__ ( "
save_frame:
	.dc.l	0
	.dc.l	0
	.ascii	\"XBRALILO\"
_old_trap1:
	.dc.l	0
_my_trap1:
	moveq	#1,d0					| trap#
	movel	pc@(_old_trap1),sp@-	| push old vector
trap_common:
	tstl	_InTrace
	jbne	3f
	btst	#5,4(sp)				| get pointer to args
	jbne	1f
	movel	usp,a0
	jbra	2f
1:	lea		sp@(12),a0
2:
	movel	a0,sp@-
	movel	d0,sp@-
	jbsr	_do_trace
	addql	#8,sp
	movel	sp@(4),save_frame		| save original frame
	movel	sp@(8),save_frame+4		| save original frame
	movel	#1f,sp@(6)				| patch in our return address
	orw		#0x2000,sp@(4)			| set supervisor bit in frame sr
3:	rts								| call old vector
1:	movel	d0,sp@-
	jbsr	_do_retval
	movel	sp@+,d0
	movel	save_frame+4,sp@-
	movel	save_frame,sp@-
	rte

	.ascii	\"XBRALILO\"
_old_trap13:
	.dc.l	0
_my_trap13:
	moveq	#13,d0					| trap#
	movel	pc@(_old_trap13),sp@-	| push old vector
	jbra	trap_common

	.ascii	\"XBRALILO\"
_old_trap14:
	.dc.l	0
_my_trap14:
	moveq	#14,d0					| trap#
	movel	pc@(_old_trap14),sp@-	| push old vector
	jbra	trap_common
");


static void do_trace( int trapno, char *argp )
{
	int i, funcnr;
	TABENT *table = NULL;
	long args[7];
	char *p;
	
	InTrace = 1;

	for( i = 0; i < sizeof(table_index)/sizeof(TABIDX); ++i ) {
		if (table_index[i].trapno == trapno) {
			table = table_index[i].table;
			break;
		}
	}
	if (!table) {
		sprintf( PrtBuf, "Unknown TRAP #%d call\n", trapno );
		bputs( PrtBuf );
		goto end;
	}

	funcnr = *((unsigned short *)argp)++;
	for( ; table->num >= 0; ++table ) {
		if (table->num == funcnr)
			break;
	}
	if (table->num < 0) {
		sprintf( PrtBuf, "Unknown function number %d for TRAP #%d\n",
				 funcnr, trapno );
		bputs( PrtBuf );
		goto end;
	}

	if (!(table->flags & TraceWhat))
		goto end;

	for( p = table->args, i = 0; *p; ++p ) {
		if (*p == 'w')
			args[i++] = *((unsigned short *)argp)++;
		else
			args[i++] = *((unsigned long *)argp)++;
	}
	vsprintf( PrtBuf, table->format, args );
	strcat( PrtBuf, " = " );
	bputs( PrtBuf );

  end:
	InTrace = 0;
}


static void do_retval( unsigned long val )
{
	int i;
	extern struct {
		long num;
		const char *str;
	} toserr_table[];

	InTrace = 1;

	sprintf( PrtBuf, "%ld %08lx", val, val );
	for( i = 0; ; ++i ) {
		if (toserr_table[i].num == (long)val) {
			strcat( PrtBuf, " (" );
			strcat( PrtBuf, toserr_table[i].str );
			strcat( PrtBuf, ")" );
			break;
		}
		if (toserr_table[i].num == -67)
			break;
	}
	strcat( PrtBuf, "\n" );
	bputs( PrtBuf );
	InTrace = 0;
}


static void bputs( const char *p )
{
	for( ; *p; ++p ) {
		if (*p == '\n')
			Bconout( 2, '\r' );
		Bconout( 2, *p );
	}
}

#endif /* STRACE_TOS */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
