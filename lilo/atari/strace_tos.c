/* strace_tos.c -- Strace-like tracing for GEMDOS system calls
 *
 * Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: strace_tos.c,v 1.3 1998-03-03 11:33:49 rnhodek Exp $
 * 
 * $Log: strace_tos.c,v $
 * Revision 1.3  1998-03-03 11:33:49  rnhodek
 * Added "unused" attrib to TraceStack, it's only referenced in asm code.
 * Make also *table global.
 * Also check TraceWhat in do_retval[_se]().
 *
 * Revision 1.2  1998/03/02 13:57:58  rnhodek
 * Several fixes to really make it work, and many additions.
 *
 * Revision 1.1  1998/02/27 10:25:25  rnhodek
 * New: GEMDOS system call tracer (experimental, for debugging).
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
static int InhibitBIOS = 0;
static char PrtBuf[256];
static char TraceStack[4096] __attribute__ ((unused));
#define TRACE_STACK_END	"_TraceStack+4096-4"

/* max. bytes to print of Fread/Frwite buffers */
#define MAX_STRLEN	40
/* each char is expanded max. by 4 (e.g. \xff); 3 is for "..." */
static char StrBuf[MAX_STRLEN*4+3+1];

typedef struct table_entry {
	int  num;
	int  flags;
	char *args;
	char *format;
} TABENT;

static TABENT gemdos_calls[] = {
	{  0, TR_PROC,		"",			"Pterm0()" },
	{  1, TR_CHAR,		"",			"Cconin()" },
	{  2, TR_CHAR,		"w",		"Cconout(0x%02x)" },
	{  3, TR_CHAR,		"", 		"Cauxin()" },
	{  4, TR_CHAR,		"w",		"Cauxout(0x%02x)" },
	{  5, TR_CHAR,		"w",		"Cprnout(0x%02x)" },
	{  6, TR_CHAR,		"w",		"Crawio(0x%02x)" },
	{  7, TR_CHAR,		"",			"Crawcin()" },
	{  8, TR_CHAR,		"",			"Cnecin()" },
	{  9, TR_CHAR,		"l",		"Cconws(\"%s\")" },
	{ 10, TR_CHAR,		"l",		"Conrs(0x%08lx (maxlen=%d))" },
	{ 11, TR_CHAR,		"",			"Cconis()" },
	{ 14, TR_DIR,		"w",		"Dsetdrv(%d)" },
	{ 16, TR_CHAR,		"",			"Cconos()" },
	{ 17, TR_CHAR,		"",			"Cprnos()" },
	{ 18, TR_CHAR,		"",			"Cauxis()" },
	{ 19, TR_CHAR,		"",			"Cauxos()" },
	{ 20, TR_MEM,		"ll",		"Maddalt(0x%08lx,0x%08lx)" },
	{ 25, TR_DIR,		"",			"Dgetdrv()" },
	{ 26, TR_FILE,		"l",		"Fsetdta(0x%08lx)" },
	{ 32, TR_PROC,		"l",		"Super(0x%08lx)" },
	{ 42, TR_DATE,		"",			"Tgetdate()" },
	{ 43, TR_DATE,		"w",		"Tsetdate(0x%04x)" },
	{ 44, TR_DATE,		"",			"Tgettime()" },
	{ 45, TR_DATE,		"w",		"Tsettime(0x%04x)" },
	{ 47, TR_FILE,		"",			"Fgetdta()" },
	{ 48, TR_PROC,		"",			"Sversion()" },
	{ 49, TR_PROC,		"lw",		"Ptermres(%d,%d)" },
	{ 54, TR_DIR,		"lw",		"Dfree(0x%08lx,%d)" },
	{ 57, TR_DIR,		"l",		"Dcreate(\"%s\")" },
	{ 58, TR_DIR,		"l",		"Ddelete(\"%s\")" },
	{ 59, TR_DIR,		"l",		"Dsetpath(\"%s\")" },
	{ 60, TR_FILE|TR_OPEN, "lw",	"Fcreate(\"%s\",0x%02x)" },
	{ 61, TR_FILE|TR_OPEN, "lw",	"Fopen(\"%s\",%d)" },
	{ 62, TR_FILE,		"w",		"Fclose(%d)" },
	{ 63, TR_IO,		"wll",		"Fread(%d,%d,0x%08lx)" },
	{ 64, TR_IO,		"wll",		"Fwrite(%d,%d,0x%08lx=\"%s\")" },
	{ 65, TR_FILE,		"l",		"Fdelete(\"%s\")" },
	{ 66, TR_FILE,		"lww",		"Fseek(%d,%d,%d)" },
	{ 67, TR_FILE,		"lww",		"Fattrib(\"%s\",%d,0x%02x)" },
	{ 68, TR_MEM,		"lw",		"Mxalloc(%d,%d)" },
	{ 69, TR_FILE,		"w",		"Fdup(%d)" },
	{ 70, TR_FILE,		"ww",		"Fforce(%d,%d)" },
	{ 71, TR_DIR,		"lw",		"Dgetpath(0x%08lx,%d)" },
	{ 72, TR_MEM,		"l",		"Malloc(%d)" },
	{ 73, TR_MEM,		"l",		"Mfree(0x%08lx)" },
	{ 74, TR_MEM,		"wll",		"Mshrink(%d,0x%08lx,%d)" },
	{ 75, TR_PROC,		"wlll",		"Pexec(%d,0x%08lx,0x%08lx,0x%08lx)" },
	{ 76, TR_PROC,		"w",		"Pterm(%d)" },
	{ 78, TR_FILE|TR_OPEN, "lw",	"Fsfirst(\"%s\",0x%02x)" },
	{ 79, TR_FILE|TR_OPEN, "",		"Fsnext()" },
	{ 86, TR_FILE,		"wll",		"Frename(%d,\"%s\",\"%s\")" },
	{ 87, TR_FILE,		"lww",		"Fdattime(0x%08lx,%d,%d)" },
	{ -1, 0,			"",			"" }
};

static TABENT bios_calls[] = {
	{  1, TR_BIOS,		"w",		"Bconstat(%d)" },
	{  2, TR_BIOS,		"w",		"Bconin(%d)" },
	{  3, TR_BIOS,		"ww",		"Bconout(%d,0x%02x)" },
	{  4, TR_BIOS,		"wlwww",	"Rwabs(%d,0x%08lx,%u,%u,%d)" },
	{  5, TR_BIOS,		"wl",		"Setexc(%d,0x%08lx)" },
	{  6, TR_BIOS,		"",			"Tickcal()" },
	{  7, TR_BIOS,		"w",		"Getbpb(%d)" },
	{  8, TR_BIOS,		"w",		"Bcostat(%d)" },
	{  9, TR_BIOS,		"w",		"Mediach(%d)" },
	{ 10, TR_BIOS,		"",			"Drvmap()" },
	{ 11, TR_BIOS,		"w",		"Kbshift(0x%04x)" },
	{ -1, 0,			"",			"" }
};

static TABENT xbios_calls[] = {
	{  0, TR_XBIOS,		"wll",		"Initmous(%d,0x%08lx,0x%08lx)" },
	{  1, TR_XBIOS,		"w",		"Ssbrk(0x%04x)" },
	{  2, TR_XBIOS,		"",			"Physbase()" },
	{  3, TR_XBIOS,		"",			"Logbase()" },
	{  4, TR_XBIOS,		"",			"Getrez()" },
	{  5, TR_XBIOS,		"llw",		"Setscreen(0x%08lx,0x%08lx,%d)" },
	{  6, TR_XBIOS,		"l",		"Setpalette(0x%08lx)" },
	{  7, TR_XBIOS,		"ww",		"Setcolor(0x%04x,%d)" },
	{  8, TR_XBIOS,		"llwwwww",	"Floprd(0x%08lx,%d,%d,%d,%d,%d)" },
	{  9, TR_XBIOS,		"llwwwww",	"Flopwr(0x%08lx,%d,%d,%d,%d,%d)" },
	{ 10, TR_XBIOS,		"llwwww",  "Flopfmt(0x%08lx,0x%08lx,%d,%d,%d,%d,...)"},
	{ 12, TR_XBIOS,		"wl",		"Midiws(%d,0x%08lx)" },
	{ 13, TR_XBIOS,		"wl",		"Mfpint(%d,0x%08lx)" },
	{ 14, TR_XBIOS,		"w",		"Iorec(%d)" },
	{ 15, TR_XBIOS,		"wwwwww", "Rsconf(%d,%d,0x%02x,0x%02x,0x%02x,0x%02x)"},
	{ 16, TR_XBIOS,		"lll",		"Keytbl(0x%08lx,0x%08lx,0x%08lx)" },
	{ 17, TR_XBIOS,		"",			"Random()" },
	{ 18, TR_XBIOS,		"llww",		"Protobt(0x%08lx,0x%08lx,%d,%d)" },
	{ 19, TR_XBIOS,		"llwwwww",	"Flopver(0x%08lx,%d,%d,%d,%d,%d)" },
	{ 20, TR_XBIOS,		"",			"Scrdmp()" },
	{ 21, TR_XBIOS,		"ww",		"Cursconf(%d,%d)" },
	{ 22, TR_XBIOS,		"l",		"Settime(0x%08lx)" },
	{ 23, TR_XBIOS,		"",			"Gettime()" },
	{ 24, TR_XBIOS,		"",			"Bioskeys()" },
	{ 25, TR_XBIOS,		"wl",		"Ikbdws(%d,0x%08lx)" },
	{ 26, TR_XBIOS,		"w",		"Jdisint(%d)" },
	{ 27, TR_XBIOS,		"w",		"Jenabint(%d)" },
	{ 28, TR_XBIOS,		"ww",		"Giaccess(0x%02x,%d)" },
	{ 29, TR_XBIOS,		"w",		"Offgibit(%d)" },
	{ 30, TR_XBIOS,		"w",		"Ongibit(%d)" },
	{ 31, TR_XBIOS,		"wwwl",		"Xbtimer(%d,0x%02x,%d,0x%08lx)" },
	{ 32, TR_XBIOS,		"l",		"Dosound(0x%08lx)" },
	{ 33, TR_XBIOS,		"w",		"Setprt(0x%02x)" },
	{ 34, TR_XBIOS,		"",			"Kdvbase()" },
	{ 35, TR_XBIOS,		"ww",		"Kbrate(%d,%d)" },
	{ 36, TR_XBIOS,		"l",		"Prtblk(0x%08lx)" },
	{ 37, TR_XBIOS,		"",			"Vsync()" },
	{ 38, TR_XBIOS,		"l",		"Supexec(0x%08lx)" },
	{ 39, TR_XBIOS,		"",			"Puntaes()" },
	{ 41, TR_XBIOS,		"ww",		"Floprate(%d,%d)" },
	{ 42, TR_XBIOS,		"lwlw",		"DMAread(%lu,%d,0x%08lx,%d)" },
	{ 43, TR_XBIOS,		"lwlw",		"DMAwrite(%lu,%d,0x%08lx,%d)" },
	{ 44, TR_XBIOS,		"w",		"Bconmap(%d)" },
	{ 46, TR_XBIOS,		"wwwl",		"NVMaccess(%d,%d,%d,0x%08lx)" },
	{ 64, TR_XBIOS,		"w",		"Blitmode(%d)" },
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

/* TOS error abbrevs, indexed by -retval */
char *tos_errs[] = {
	NULL,			/* 0 */
	"ERROR",
	"EDRVNR",
	"EUNCMD",
	"E_CRC",
	"EBADRQ",		/* -5 */
	"E_SEEK",
	"EMEDIA",
	"ESECNF",
	"EPAPER",
	"EWRITF",		/* -10 */
	"EREADF",
	"EGENRL",
	"EWRPRO",
	"E_CHNG",
	"EUNDEV",		/* -15 */
	"EBADSF",
	"EOTHER",
	"EINSERT",
	"EDRVRSP",
	NULL,			/* -20 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,			/* -25 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,			/* -30 */
	NULL,
	"EINVFN",
	"EFILNF",
	"EPTHNF",
	"ENHNDL",		/* -35 */
	"EACCDN",
	"EIHNDL",
	NULL,
	"ENSMEM",
	"EIMBA",		/* -40 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,			/* -45 */
	"EDRIVE",
	NULL,
	"ENSAME",
	"ENMFIL",
	NULL,			/* -50 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,			/* -55 */
	NULL,
	NULL,
	"ELOCKED",
	"ENSLOCK",
	NULL,			/* -60 */
	NULL,
	NULL,
	NULL,
	"ERANGE",
	"EINTRN",		/* -65 */
	"EPLFMT",
	"EGSBF"
};

/***************************** Prototypes *****************************/

static void install_vectors( void );
static void uninstall_vectors( void );
static int remove_vector( unsigned long *vec, const char *vecname );
static void do_trace( int trapno, char *args )
	__attribute__ ((unused));
static void do_retval( long val );
static void do_retval_se( long val )
	__attribute__ ((unused));
static void bputs( const char *p );
static char *format_str( char *str, int len );

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
save_addr:
	.dc.l	0
save_addr_se:
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
	movel	usp,a0
	btst	#5,4(sp)				| get pointer to args (different for
	jbeq	1f						| user/super mode)
	lea		sp@(12),a0
1:
| Switch to our own stack. If we don't do this, we overwrite lots of the stack
| where the previous SSP pointed to. And for new processes, this is default
| BIOS stack that was also used by the root process. And on this stack the
| registers of the initial Pexec are saved.
	movel	sp,"TRACE_STACK_END"
	movel	#"TRACE_STACK_END",sp

	movel	a0,sp@-					| call do_trace()
	movel	d0,sp@-
	jbsr	_do_trace
	movel	sp@+,d0
	movel	sp@+,a0
	movel	sp@+,sp

	cmpw	#1,d0					| test for Pexec modes 0, 4, or 6
	jbne	1f						| for those, don't modify the frame
	cmpw	#75,a0@
	jbne	1f
	cmpw	#0,a0@(2)
	jbeq	3f
	cmpw	#4,a0@(2)
	jbeq	3f
	cmpw	#6,a0@(2)
	jbeq	3f

1:	cmpw	#14,d0					| test for Supexec
	jbne	1f
	cmpw	#38,a0@
	jbne	1f
	
	movel	sp@(6),save_addr_se		| save return addr to special location
	movel	#4f,sp@(6)				| patch in Supexec return address
	jbra	3f

1:	movel	sp@(6),save_addr		| save original return addr
	movel	#1f,sp@(6)				| patch in our return address
3:	rts								| call old vector

| Here we're in user mode again, so we can't do rte at the end and must use
| rts instead. This clobbers the ccr value, but who cares?
1:
	movel	sp,"TRACE_STACK_END"
	movel	#"TRACE_STACK_END",sp
	movel	d0,sp@-					| save retval and pass as arg
	jbsr	_do_retval				| call do_retval()
	movel	sp@+,d0					| restore retval in d0
	movel	sp@+,sp
	movel	save_addr,sp@-			| push old return addr on stack
	rts								| return to caller

4:
	movel	sp,"TRACE_STACK_END"
	movel	#"TRACE_STACK_END",sp
	movel	d0,sp@-					| save retval and pass as arg
	jbsr	_do_retval_se			| call do_retval()
	movel	sp@+,d0					| restore retval in d0
	movel	sp@+,sp
	movel	save_addr_se,sp@-		| push old return addr on stack
	rts								| return to caller


	.ascii	\"XBRALILO\"
_old_trap13:
	.dc.l	0
_my_trap13:
	moveq	#13,d0					| trap#
	movel	pc@(_old_trap13),sp@-	| push old vector
	tstl	_InhibitBIOS
	jbeq	trap_common
	rts

	.ascii	\"XBRALILO\"
_old_trap14:
	.dc.l	0
_my_trap14:
	moveq	#14,d0					| trap#
	movel	pc@(_old_trap14),sp@-	| push old vector
	tstl	_InhibitBIOS
	jbeq	trap_common
	rts
");


/* global so that do_retval can use them, too */
static int funcnr;
static long args[7];
static TABENT *table = NULL;


static void do_trace( int trapno, char *_argp )
{
	char *argp = _argp; /* make a copy to leave arg untouched */
	int i;
	char *p;
	int skip_std_fmt = 0;
	
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
		return;
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
		return;
	}
	/* encode trapno into funcnr */
	funcnr += (trapno-1)*1000;

	if (!(table->flags & TraceWhat))
		return;

	for( p = table->args, i = 0; *p; ++p ) {
		if (*p == 'w')
			args[i++] = *((unsigned short *)argp)++;
		else
			args[i++] = *((unsigned long *)argp)++;
	}

	/* some functions with special formats */
	if (funcnr == 9) {
		/* Cconws */
		sprintf( PrtBuf, table->format,
				 format_str( (char *)args[0], strlen((char *)args[0]) ));
		skip_std_fmt = 1;
	}
	else if (funcnr == 10) {
		/* Cconrs */
		sprintf( PrtBuf, table->format, args[0], *(unsigned char *)(args[0]) );
		skip_std_fmt = 1;
	}
	else if (funcnr == 64) {
		/* Fwrite */
		args[3] = (long)format_str( (char *)args[2], args[1] );
	}
	else if (funcnr == 75) {
		/* Pexec */
		if (args[0] == 0 || args[0] == 3 || args[0] == 5) {
			if (!args[1])
				args[1] = (long)"(NULL)";
			if (!args[2])
				args[2] = (long)"(NULL)";
			else
				args[2] += 1; /* skip length of cmdline */
			if (!args[3])
				args[3] = (long)"(NULL)";
			/* bug: only first string of env. printed */
			vsprintf( PrtBuf, "Pexec(%d,\"%s\",\"%s\",\"%s\",)", args );
			skip_std_fmt = 1;
		}
		else if (args[0] == 4 || args[0] == 6) {
			vsprintf( PrtBuf, "Pexec(%d,%d,0x%08lx,%d)", args );
			skip_std_fmt = 1;
		}
		else if (args[0] == 7) {
			if (!args[2])
				args[2] = (long)"(NULL)";
			else
				args[2] += 1; /* skip length of cmdline */
			if (!args[3])
				args[3] = (long)"(NULL)";
			/* bug: only first string of env. printed */
			vsprintf( PrtBuf, "Pexec(%d,0x%08lx,\"%s\",\"%s\",)", args );
			skip_std_fmt = 1;
		}
	}
	else if (funcnr == 87) {
		/* Fdatime */
		if (args[2] == 1) {
			sprintf( PrtBuf, "Fdattime(0x%08lx={0x%04x,0x%04x},%ld,%ld)",
					 args[0],
					 ((u_short *)args[0])[0], ((u_short *)args[0])[1],
					 args[1], args[2] );
			skip_std_fmt = 1;
		}
	}
	if (!skip_std_fmt)
		vsprintf( PrtBuf, table->format, args );

	if (funcnr == 0 || funcnr == 49 || funcnr == 76) {
		/* Pterm0, Pterm and Ptermres don't have return values */
		strcat( PrtBuf, "\n" );
		bputs( PrtBuf );
		InTrace = 0;
		return;
	}
	else if (funcnr == 75 && (args[0] == 0 || args[0] == 4 || args[0] == 6)) {
		/* Pexec modes 0,4,6 start another process and we don't trap the
		 * return value. (The user should look at the matching Pterm). */
		strcat( PrtBuf, " = ...\n(new process)\n" );
		bputs( PrtBuf );
		InTrace = 0;
		/* don't trace BIOS calls done by Pexec() */
		InhibitBIOS = 1;
		return;
	}
	else if (funcnr == 13038) {
		/* Supexec: allow tracing calls from in there */
		strcat( PrtBuf, " = ...\n" );
		bputs( PrtBuf );
		InTrace = 0;
		return;
	}
		
	strcat( PrtBuf, " = " );
	bputs( PrtBuf );
	InhibitBIOS = 0;
}


static void do_retval( long val )
{
	int skip_std_fmt = 0;
	
	if (!(table->flags & TraceWhat))
		return;

	/* catch some special cases */
	if (funcnr == 10) {
		/* Cconrs */
		int len = *(unsigned char *)(args[0]+1);
		strcpy( PrtBuf, "\"" );
		strncat( PrtBuf, (char *)args[0]+2, len );
		PrtBuf[1+len] = 0;
		strcat( PrtBuf, "\"\n" );
		skip_std_fmt = 1;
	}
	else if (funcnr == 54 && val == 0) {
		/* Dfree */
		long *p = (long *)(args[0]);
		vsprintf( PrtBuf, "{free=%ld,tot=%ld,secsiz=%ld,clsiz=%ld}\n", p );
		skip_std_fmt = 1;
	}
	else if (funcnr == 63 && val > 0) {
		/* Fread */
		sprintf( PrtBuf, "%ld \"%s\"\n", val,
				 format_str( (char *)args[2], val ));
		skip_std_fmt = 1;
	}
	else if (funcnr == 71 && val == 0) {
		/* Dgetpath */
		sprintf( PrtBuf, "\"%s\"\n", (char *)args[0] );
		skip_std_fmt = 1;
	}
	else if ((funcnr == 78 || funcnr == 79) && val == 0) {
		/* Fsfirst */
		_DTA *dta = Fgetdta();
		sprintf( PrtBuf, "{attr=0x%02x,time=0x%04x,date=0x%04x,"
				 "size=%ld,name=\"%s\"}\n",
				 dta->dta_attribute, dta->dta_time, dta->dta_date,
				 dta->dta_size, dta->dta_name );
		skip_std_fmt = 1;
	}
	else if (funcnr == 87 && args[2] == 0) {
		/* Fdatime */
		u_short *p = (u_short *)args[0];
		vsprintf( PrtBuf, "{time=0x%04x,date=0x%04x}\n", p );
	}

	if (!skip_std_fmt) {
		if (val < 0 && -val <= arraysize(tos_errs) && tos_errs[-val])
			sprintf( PrtBuf, "%ld (%s)\n", val, tos_errs[-val] );
		else if (val >= -256 && val < 256)
			sprintf( PrtBuf, "%ld\n", val );
		else
			sprintf( PrtBuf, "0x%08lx\n", (u_long)val );
	}
	
	bputs( PrtBuf );
	InTrace = 0;
}

/* Special version for return from Supexec() */
static void do_retval_se( long val )
{	
	if (!(table->flags & TraceWhat))
		return;

	bputs( "Supexec(...) = " );
	do_retval( val );
}


static void bputs( const char *p )
{
	for( ; *p; ++p ) {
		if (*p == '\n')
			Bconout( 2, '\r' );
		Bconout( 2, *p );
	}
}

static char *format_str( char *str, int len )
{
	static char ctrl_trans[32] =
		"0\0\0\0\0\0\0ab\0n\0fr\0\0\0\0\0\0\0\0\0\0\0\0\0e\0\0\0\0";
	char *p = StrBuf;
	int i;
	
	for( i = 0; i < MAX_STRLEN && i < len; ++i, ++str ) {
		switch( (unsigned char)*str ) {
		  case 0 ... 31:
			if (ctrl_trans[(int)*str]) {
				*p++ = '\\';
				*p++ = ctrl_trans[(int)*str];
				break;
			}
			/* else: fall through to general non-printable case */
		  case 127 ... 255:
			sprintf( p, "\\x%02x", (unsigned char)*str );
			p += 4;
			break;

		  default:
			*p++ = *str;
		}
	}

	if (len > MAX_STRLEN) {
		*p++ = '.';
		*p++ = '.';
		*p++ = '.';
	}
	
	*p = 0;
	return( StrBuf );
}

#endif /* STRACE_TOS */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
