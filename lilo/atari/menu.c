/* menu.c -- Display boot menu for Atari LILO
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: menu.c,v 1.9 1998-03-10 10:29:43 rnhodek Exp $
 * 
 * $Log: menu.c,v $
 * Revision 1.9  1998-03-10 10:29:43  rnhodek
 * Make serial_instat and serial_getc global functions.
 * dokey(): Backspace: print backspace only if deleting actually done;
 * ^U: test for len > 0, remove buggy ';' after while.
 * read_line(): ^U: test for len > 0.
 * waitkey(): New function.
 * serial_getc(): Backspace also returns scancode part; also allow ctrl
 * chars CR and LF.
 * serial_putc(): Remove special handling of \b, it's not only necessary
 * on the serial console.
 *
 * Revision 1.8  1998/03/04 09:20:16  rnhodek
 * goto_last_line: Must use VDI function as long as workstation is open,
 * VT52 escapes don't work.
 *
 * Revision 1.7  1998/03/02 13:12:24  rnhodek
 * #ifdef out many parts if NO_GUI is defined.
 * Rename NoGUI to DontUseGUI.
 * Remove unused line() function.
 * Write mode for slanted text must be MD_TRANS (instead of MD_REPLACE),
 * otherwise parts of the previous chars were overwritten; (new function
 * writemode() for this).
 * graf_deinit(): Don't modify alpha cursor position.
 * goto_last_line(): New function for that, which can be called at any time.
 *
 * Revision 1.6  1998/02/26 11:20:32  rnhodek
 * If v_opnwk fails, don't exit but switch to NoGUI mode.
 * Replace some more printf's by cprintf.
 *
 * Revision 1.5  1998/02/26 10:23:27  rnhodek
 * New 'timeout_canceled' that is set after the first keypress, mouse
 * move, or receive from the serial. The timeout should fire only if
 * there wasn't any user action at all.
 * Respect NoGUI in menu_error() and get_password().
 * Fix going to last line of screen in graf_deinit().
 * Turn on and off cursor in read_line().
 * Fix handling of Backspace in read_line().
 * Make read_line() also obey EchoMode, for reading passwords.
 *
 * Revision 1.4  1998/02/25 10:37:36  rnhodek
 * New argument 'doprompt' to read_line().
 * Use correct length in show_cmdline().
 * Make asm in v_gtext() look nicer.
 * In read_line():
 *  - Print backspaces as "\b \b" in case \b only goes back but doesn't delete.
 *  - Case for ASC_CR was missing.
 *  - Echo was missing for normal characters.
 *  - Fix wrong usage of sizeof.
 *
 * Revision 1.3  1997/09/19 09:06:59  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.2  1997/08/23 20:48:04  rnhodek
 * Added some debugging printf
 *
 * Revision 1.1  1997/08/12 15:27:10  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <osbind.h>
#include <vdibind.h>
#include <gemfast.h>

/* define if running as GEM program and opening a physical workstation isn't
 * possible */
#undef AES

#include "config.h"
#include "loader.h"
#include "menu.h"
#include "sysvars.h"

enum echomode {
	ECHO_NORMAL,
	ECHO_STARS
} EchoMode = ECHO_NORMAL;

/* the command line entered by the user */
static char cmdline[129];
#ifndef NO_GUI
/* for initializing the command line display */
static char *underscores = "___________________________________________________________________________________________________________________________________";
static char *stars = "***********************************************************************************************************************************";
/* the cursor position */
static int cursor;

/* icon images, defined in icon.c */
extern MFDB iconMFDB[3];

/* various screen parameters */
static int grh, scr_w, scr_h, charw, lineht, icon;
static int hcmdo, vcmd, hcmdc, vtop, vhei, hborder, hwid, hoff;

/* flag whether phys. VDI workstation is open */
static int workstation_open = 0;
#endif /* NO_GUI */

/* set after first keypress or mouse move */
static int timeout_canceled = 0;

/* various definitions for key codes*/
#define	CTRL(x)		((x) & 0x1f)

#define	ASC_BS		8
#define	ASC_TAB		9
#define	ASC_LF		10
#define	ASC_CR		13
#define	ASC_DEL		127
#define	CTRL_A		CTRL('a')
#define	CTRL_B		CTRL('b')
#define	CTRL_D		CTRL('d')
#define	CTRL_E		CTRL('e')
#define	CTRL_F		CTRL('f')
#define	CTRL_K		CTRL('k')
#define	CTRL_U		CTRL('u')
#define	CTRL_X		CTRL('x')
#define	SCAN_ESC	1
#define	SCAN_BS		14
#define	SCAN_B		48
#define	SCAN_D		32
#define	SCAN_F		33
#define	SCAN_F1		59
#define	SCAN_F8		66
#define	SCAN_HOME	71
#define	SCAN_LEFT	75
#define	SCAN_RIGHT	77
#define	SCAN_INS	82
#define	SCAN_DEL	83
#define	SCAN_SF1	84
#define	SCAN_SF8	91
#define	SCAN_CLEFT	115
#define	SCAN_CRIGHT	116
#define	SCAN_CEND	117
#define	SCAN_CHOME	119

#define	IS_SHIFT	((shift & 3) != 0)
#define	IS_CTRL		((shift & 4) != 0)
#define	IS_ALT		((shift & 8) != 0)

#ifndef NO_GUI
/* maximum width of a label */
#define	LABELWIDTH	16
#define	NLABELS		16
static char labels[NLABELS][LABELWIDTH+1];

/* screen layout parameters */
#define	BUTTONLINES		8		/* number of button lines */
#define	BUTTONCOLS		2		/* number of bottom column */
#define	OVERBUTTONLINES	2		/* empty border text lines above buttons */
#endif /* NO_GUI */


/***************************** Prototypes *****************************/

#ifndef NO_GUI
static int init_labels( const char *dflt_label );
static int dokey( int key, int shift );
static int domouse( int x, int y );
static void init_editor( void );
static void set_cmdline( const char *newline );
static void move_cursor( int new );
static int next_word( void );
static int previous_word( void );
static void delete( int dst, int src, int len );
static void show_cmdline( int start, int end, int newc );
#endif /* NO_GUI */
static void serial_putc( char c );
static void serial_puts( const char *p );
#ifndef NO_GUI
static void text( int x, int y, const char *str );
static void texta( int x, int y, int halign, int valign, const char *str );
static void texteffect( int effect );
static void textrotate( int deg );
static void textsize( int dbl );
static void text_extent( const char *str, int *w, int *h );
static void writemode( int mode );
static void box( int x, int y, int w, int h, int linew );
static void copyicon( int x, int y );
static int getmouse( int *x, int *y );
static void reverse( void );
static void clear( int x, int y, int w, int h );
int v_gtext( int h, int x, int y, char *s );
#endif /* NO_GUI */

/************************* End of Prototypes **************************/



char *boot_menu( const char *dflt_label )
{
#ifndef NO_GUI
	int i, j, idx, x, y, w, h, lowwidth, labelwidth;
	int vsep, hcmdw, lower_border, mid_lefthalf, uplef_space;
	int dflt_index;
	unsigned long timeout = 0;
#endif /* NO_GUI */

	if (DontUseGUI) {
		read_line( 1, 1 );
		if (!cmdline[strspn( cmdline, " " )])
			strcpy( cmdline, dflt_label );
		return( cmdline );
	}
	
#ifndef NO_GUI
	dflt_index = init_labels( dflt_label );
	
	lowwidth = (scr_w / charw) < 80;
	lower_border = scr_h - 4*lineht;
	if (lower_border > 18*lineht && 3*scr_h/4 > 18*lineht)
		lower_border = 3*scr_h/4;

	hborder = scr_w / 3;
	mid_lefthalf = hborder / 2;

	if (scr_h < 400)
		icon = 0;
	else if (scr_h < 800)
		icon = 1;
	else
		icon = 2;

	/* title */
	textrotate( 90 );
	textsize( 1 );
	texteffect( 5 );
	text_extent( "Atari", &w, &h );
	uplef_space = (lower_border - iconMFDB[icon].fd_h - h) / 3;
	writemode( MD_TRANS ); /* needed for slanted text */
	texta( mid_lefthalf, uplef_space, 1, 3, "Atari" );
	texteffect( 20 );
	texta( mid_lefthalf, uplef_space, 1, 5, "LILO" );
	writemode( MD_REPLACE );
	textrotate( 0 );
	textsize( 0 );
	texteffect( 0 );
	copyicon( mid_lefthalf-iconMFDB[icon].fd_w/2, h+2*uplef_space );

	/* buttons */
	labelwidth = lowwidth ? 8 : 16;
	vsep = (lower_border-BUTTONLINES*lineht)/(BUTTONLINES-1+2*OVERBUTTONLINES);
	vhei = vsep + lineht;
	vtop = OVERBUTTONLINES*vsep;
	hwid = (scr_w - hborder) / BUTTONCOLS;
	hoff = (hwid - (labelwidth+2)*charw) / 2;
	for( i = 0; i < BUTTONCOLS; ++i ) {
		for( j = 0; j < BUTTONLINES; ++j ) {
			idx = i*BUTTONLINES+j;
			if (!*labels[idx]) continue;
			if (strlen(labels[idx]) > labelwidth)
				labels[idx][labelwidth] = 0;
			box( hborder + i*hwid + hoff, vtop + j*vhei,
			     (labelwidth+2)*charw, lineht,
			     idx == dflt_index ? 3 : 2 );
			texta( hborder + i*hwid + hwid/2, vtop + j*vhei,
				   1, 5, labels[idx] );
			if (hoff >= 4*charw) {
				char key[4];
				sprintf( key, "%sF%d", i == 1 ? "\001" : " ", j+1 );
				text( hborder + i*hwid + hoff - 4*charw, vtop + j*vhei,
					  key );
			}
		}
	}

	/* command line */
	vcmd = lower_border + (scr_h - lower_border) / 2;
	hcmdw = scr_w - 4*charw;
	if (hcmdw > 128*charw)
		hcmdw = 128*charw;
	hcmdc = hcmdw / charw;
	hcmdo = (scr_w - hcmdw)/2;
	box( hcmdo-charw, vcmd-lineht, hcmdw+2*charw, 2*lineht, 2 );
	text( hcmdo, vcmd-lineht, "Command line:" );

	init_editor();	
	v_show_c( grh, 0 );

	if (BootOptions->TimeOut)
		timeout = _hz_200 + *BootOptions->TimeOut * HZ;

	while( 1 ) {
		if (Cconis()) {
			u_long key;
			timeout_canceled = 1;
			/* make sure Kbshift() status matches status at the time the key
			 * was pressed, so don't put both calls into the arg list */
			key = Crawcin();
			if (dokey( key, Kbshift(-1) ))
				break;
		}
		if (serial_instat()) {
			timeout_canceled = 1;
			if (dokey( serial_getc(), 0 ))
				break;
		}
		if (getmouse( &x, &y ) & 1) {
			timeout_canceled = 1;
			if (domouse( x, y ))
				break;
		}
		if (!timeout_canceled && BootOptions->TimeOut && _hz_200 >= timeout) {
			/* timed out, use default OS */
			set_cmdline( labels[dflt_index] );
			AutoBoot = 1;
			break;
		}
	}

	v_hide_c( grh );
	return( cmdline );
#endif /* NO_GUI */
}

/*
 * Show an error message
 */
void menu_error( const char *str )
{
#ifndef NO_GUI
	int x, y;
	unsigned long to;
#endif /* NO_GUI */

	if (DontUseGUI) {
		cprintf( "%s\n", str );
		return;
	}
	
#ifndef NO_GUI
	y = vcmd+lineht;
	y += (scr_h - y) / 2;
	x = (scr_w - strlen(str)*charw)/2;
	texteffect( 1 );
	text( x, y, str );
	texteffect( 0 );
	Cconout( 7 );
	for( to = _hz_200 + 200; _hz_200 < to; )
		;
	clear( x, y, strlen(str)*charw, lineht );
#endif /* NO_GUI */
}

/*
 * Let the user enter a password
 */
char *get_password( void )
{
	EchoMode = ECHO_STARS;
	
	if (DontUseGUI) {
		cprintf( "Password: " );
		read_line( 1, 0 );
	}
	else {
#ifndef NO_GUI
		text( hcmdo, vcmd-lineht, "Password:    " );
		init_editor();
		while( 1 ) {
			if (Cconis())
				if (dokey( Crawcin(), Kbshift(-1) ))
					break;
			if (serial_instat()) {
				if (dokey( serial_getc(), 0 ))
					break;
			}
		}
#endif /* NO_GUI */
	}

	EchoMode = ECHO_NORMAL;
	return( cmdline );
}

#ifndef NO_GUI

/*
 * Initialize labels[] array with labels of boot records
 */
static int init_labels( const char *dflt_label )
{
	struct BootRecord *p;
	unsigned int i;
	int dflt_idx = -1;

	/* copy labels from boot records, limit length to LABELWIDTH */
	for( p = BootRecords, i =0; p && i < NLABELS; p = p->Next, ++i ) {
		strncpy( labels[i], p->Label, LABELWIDTH );
		labels[i][LABELWIDTH] = 0;
		if (dflt_idx < 0 && strcmp( p->Label, dflt_label ) == 0)
			dflt_idx = i;
	}
	/* fill rest of labels with empty strings */
	for( ; i < NLABELS; ++i )
		labels[i][0] = 0;

	return( dflt_idx );
}

/*
 * Initialize VDI workstation and interrogate necessary screen parameters
 */
void graf_init( const unsigned long *video_res )
{
	int i, work_in[11], work_out[57];

	if (DontUseGUI)
		return;
	if (Debug)
		cprintf( "Initializing VDI workstation\n" );

#ifdef AES
	appl_init();
	grh = graf_handle( &dum, &dum, &dum, &dum );
#endif

	/* prepare work_in array: select resolution, everything else default */
	work_in[0] = video_res ? *video_res + 2 : 1;
	for( i = 1; i < 10; ++i )
		work_in[i] = 1;
	work_in[10] = 2;

	/* now open the workstation */
#ifdef AES
	v_opnvwk( work_in, &grh, work_out );
#else
	v_opnwk( work_in, &grh, work_out );
#endif
	if (!grh) {
    	cprintf( "Error: Cannot open VDI workstation (v_opnwk failed)\n"
			     "NoGUI mode enabled.\n" );
		DontUseGUI = 1;
		return;
	}
	workstation_open = 1;
	/* number of pixels on the screen */
	scr_w = work_out[0]+1;
	scr_h = work_out[1]+1;

	/* get width and height of character cell */
	vqt_attributes( grh, work_out );
	charw  = work_out[8];
	lineht = work_out[9];

	/* set some defaults for drawing parameters */
	vswr_mode( grh, MD_REPLACE );  
	vsl_width( grh, 1 );
	vsl_ends( grh, 0, 0 );
	vsf_color( grh, 1 );
	vsf_interior( grh, 1 );

	/* hide mouse cursor and clear screen */
	v_hide_c( grh );	
	v_clrwk( grh  );
}

#endif /* NO_GUI */

/*
 * Close phys. VDI workstation, if open
 */
void graf_deinit( void )
{
#ifndef NO_GUI
	if (workstation_open) {
		v_clswk( grh );
		workstation_open = 0;
		if (Debug)
			cprintf( "Closed VDI workstation\n" );
	}
#endif /* NO_GUI */
}

/* Goto last screen line by repeating ESC B; this goes down one line except
 * already at the bottom. Assuming that the screen hasn't more than 160
 * lines... :-) */
void goto_last_line( void )
{
	int i;

#ifndef NO_GUI
	if (workstation_open) {
		for( i = 0; i < 160; ++i )
			v_curdown( grh );
	}
	else {
#endif
		for( i = 0; i < 160; ++i )
			Cconws( "\033B" );
#ifndef NO_GUI
	}
#endif
}

#ifndef NO_GUI

/*
 * Process a key press
 */
static int dokey( int key, int shift )
{
	int asc = key & 0xff;
	int scan = (key >> 16) & 0xff;
	int len = strlen(cmdline);
	int np;

	if (scan == SCAN_BS && (IS_ALT || IS_CTRL)) {
		/* delete previous word */
		np = previous_word();
		if (np < cursor)
			delete( np, cursor, len );
	}
	if (scan == SCAN_BS && IS_SHIFT) {
		/* delete to start of line */
		if (cursor > 0)
			delete( 0, cursor, len );
	}
	else if (scan == SCAN_BS) {
		/* delete char before cursor */
		if (cursor > 0) {
			delete( cursor-1, cursor, len );
			serial_puts( "\b \b" );
		}
	}
	else if ((scan == SCAN_DEL && (IS_ALT || IS_CTRL)) ||
			 (scan == SCAN_D && IS_ALT)) {             
		/* delete next word */
		np = next_word();
		if (np > cursor)
			delete( cursor, np, len );
	}
	else if ((scan == SCAN_DEL && IS_SHIFT) || asc == CTRL_K) {
		/* delete to end of line */
		if (cursor < len) {
			cmdline[cursor] = 0;
			show_cmdline( cursor, 999, cursor );
		}
	}
	else if (scan == SCAN_DEL || asc == CTRL_D) {
		/* delete char under cursor */
		if (cursor < len)
			delete( cursor, cursor+1, len );
	}
	else if (scan == SCAN_ESC || asc == CTRL_U || asc == CTRL_X) {
		/* delete whole line */
		if (len > 0) {
			cmdline[0] = 0;
			show_cmdline( 0, len, 0 );
			while( len-- )
				serial_puts( "\b \b" );
		}
	}
	else if ((scan == SCAN_HOME && !IS_SHIFT) ||
			 (scan == SCAN_LEFT && IS_SHIFT) ||
			 scan == SCAN_CHOME || asc == CTRL_A) {
		/* go to start of line */
		move_cursor( 0 );
	}
	else if ((scan == SCAN_HOME && IS_SHIFT) ||
			 (scan == SCAN_RIGHT && IS_SHIFT) ||
			 scan == SCAN_CEND || asc == CTRL_E) {
		/* go to end of line */
		move_cursor( len );
	}
	else if ((scan == SCAN_CLEFT && IS_CTRL) ||
			 (scan == SCAN_B && IS_ALT)) {
		/* go to previous word */
		move_cursor( previous_word() );
	}
	else if ((scan == SCAN_CRIGHT && IS_CTRL) ||
			 (scan == SCAN_F && IS_ALT)) {
		/* go to next word */
		move_cursor( next_word() );
	}
	else if (scan == SCAN_LEFT || asc == CTRL_B) {
		if (cursor > 0)
			move_cursor( cursor-1 );
	}
	else if (scan == SCAN_RIGHT || asc == CTRL_F) {
		if (cursor < len)
			move_cursor( cursor+1 );
	}
	else if ((scan >= SCAN_F1 && scan <= SCAN_F8) ||
		     (scan >= SCAN_SF1 && scan <= SCAN_SF8)) {
		/* function keys F1..F8 and S-F1..S-F8 */
		/* no func keys while entering password */
		if (EchoMode == ECHO_NORMAL) {
			int idx;
			if (scan <= SCAN_F8)
				idx = scan - SCAN_F1;
			else
				idx = scan - SCAN_SF1 + 8;
			if (!*labels[idx]) {
				Cconout( 7 );
				return( 0 );
			}
			set_cmdline( labels[idx] );
			return( 1 );
		}
	}
	else if (asc == ASC_CR || asc == ASC_LF) {
		/* return or linefeed -> end input */
		serial_putc( '\n' );
		return( 1 );
	}
	else if (asc >= ' ' || asc == ASC_TAB) {
		/* ordinary character */
		if (len == hcmdc) {
			Cconout( 7 );
			serial_putc( 7 );
		}
		else {
			if (asc == ASC_TAB)
				asc = ' ';
			memmove( cmdline+cursor+1, cmdline+cursor,
			         strlen(cmdline+cursor)+1 );
			cmdline[cursor] = asc;
			show_cmdline( cursor, len+1, cursor+1 );
			serial_putc( EchoMode == ECHO_STARS ? '*' : asc );
		}
	}
	
	return( 0 );
}

/*
 * Process a mouse button press
 */
static int domouse( int x, int y )
{
	int xoff, yoff, idx;

	/* first move coordinates and test for enclosing rectangle of all
	 * buttons */
	x -= hborder;
	y -= vtop;
	if (x < 0 || y < 0 ||
	    x >= BUTTONCOLS*hwid || y >= BUTTONLINES*vhei) {
		return( 0 );
	}

	/* get column/line number and offset inside there */
	xoff = x % hwid;
	x /= hwid;
	yoff = y % vhei;
	y /= vhei;
	if (xoff < hoff || xoff >= hwid-hoff ||
	    yoff < 0 || yoff >= lineht)
		return( 0 );

	idx = x*BUTTONLINES + y;
	if (!*labels[idx])
		return( 0 );
	
	set_cmdline( labels[idx] );
	return( 1 );
}



/* ------------------------------------------------------------------------- */
/*						   Editor Utility Functions							 */

/*
 * Initialize the command line editor
 */
static void init_editor( void )
{
	cmdline[0] = 0;
	show_cmdline( 0, 999, 0 );

	/* on serial port, show prompt */
	if (SerialPort)
		serial_puts( (EchoMode == ECHO_STARS) ? "Password: " : Prompt );
}

/*
 * Fill complete command line with a new string and redisplay
 */
static void set_cmdline( const char *newline )
{
	strcpy( cmdline, newline );
	show_cmdline( 0, 999, strlen(cmdline) );
}


/*
 * Move text cursor to a new position
 */
static void move_cursor( int new )
{
	v_hide_c(grh);
	reverse();
	cursor = new;
	reverse();
	v_show_c(grh,1);
}

/*
 * get offset of next word after cursor
 */
static int next_word( void )
{
	char *p = &cmdline[cursor];
	
	while( *p && *p == ' ' )
		++p;
	while( *p && *p != ' ' )
		++p;
	return( p - cmdline );
}

/*
 * get offset of word before cursor
 */
static int previous_word( void )
{
	char *p = &cmdline[cursor];
	
	while( p > cmdline && p[-1] == ' ' )
		--p;
	while( p > cmdline && p[-1] != ' ' )
		--p;
	return( p - cmdline );
}

/*
 * Delete characters between 'dst' (incl.) and 'src' (excl.) and redisplay;
 * 'len' is length of command line
 */
static void delete( int dst, int src, int len )
{
	memmove( cmdline + dst, cmdline + src, len-dst );
	show_cmdline( dst, len, dst );
}

/*
 * Redisplay command line between offsets 'start' and 'end' and set cursor
 * position to 'newc'
 */
static void show_cmdline( int start, int end, int newc )
{
	char *line = (EchoMode == ECHO_STARS) ? stars : cmdline;
	int len = strlen(cmdline), savec;

	v_hide_c(grh);
	reverse();
	
	if (end > hcmdc)
		end = hcmdc;
	
	if (start < len) {
		savec = line[end];
		line[end] = 0;
		text( hcmdo + start*charw, vcmd, line+start );
		line[end] = savec;
	}
	
	if (end > len) {
		underscores[end-len] = 0;
		text( hcmdo + len*charw, vcmd, underscores );
		underscores[end-len] = '_';
	}

	cursor = newc;
	reverse();
	v_show_c(grh,1);
}

#endif /* NO_GUI */

/* ------------------------------------------------------------------------- */
/*								Non-GUI editor								 */


/*
 * Read a line from a "dumb" terminal; only editing is BS or DEL (both delete
 * backwards), and ^U (deletes whole line)
 */
char *read_line( int dotimeout, int doprompt )
{
	unsigned long timeout = 0;
	unsigned int len = 0;
	char c;
	
	*cmdline = 0;
	if (doprompt)
		cprintf( "%s", Prompt );
	printf( "\033e" ); fflush(stdout); /* enable cursor */

	if (dotimeout && BootOptions->TimeOut)
		timeout = _hz_200 + *BootOptions->TimeOut * HZ;
	
	while( 1 ) {
		if (Cconis()) {
			timeout_canceled = 1;
			c = Crawcin();
			goto process_key;
		}
		if (serial_instat()) {
			timeout_canceled = 1;
			c = serial_getc();
		  process_key:
			if ((c == ASC_BS || c == ASC_DEL) && len > 0) {
				cmdline[--len] = 0;
				cprintf( "\b \b" );
			}
			else if (c == CTRL_U && len > 0) {
				while( len-- )
					cprintf( "\b \b" );
				*cmdline = 0;
			}
			else if (c == ASC_CR || c == ASC_LF) {
				cprintf( "\n" );
				break;
			}
			else if (c >= ' ' || c == ASC_TAB) {
				if (len == sizeof(cmdline)-1)
					cprintf( "\a" );
				else {
					if (c == ASC_TAB)
						c = ' ';
					cprintf( "%c", EchoMode == ECHO_STARS ? '*' : c );
					cmdline[len++] = c;
					cmdline[len] = 0;
				}
			}
		}
		if (dotimeout && !timeout_canceled && BootOptions->TimeOut &&
			_hz_200 >= timeout) {
			/* timed out, use default OS by empty line */
			*cmdline = 0;
			AutoBoot = 1;
			break;
		}
	}

	printf( "\033f" ); fflush(stdout); /* disable cursor */
	return( cmdline );
}


/*
 * Wait for a key
 */
int waitkey( void )
{
	for(;;) {
		if (Cconis())
			return( Crawcin() & 0xff );
		if (serial_instat())
			return( serial_getc() );
	}
}

/*
 * Print a string both on the console and the serial terminal
 */
void cprintf( const char *format, ... )
{
	static char buf[256];
	vsprintf( buf, format, &format+1 );
	fputs( buf, stdout ); fflush( stdout );
	serial_puts( buf );
}


/* ------------------------------------------------------------------------- */
/*							Serial Port Functions							 */

int serial_instat( void )
{
	return( SerialPort ? Bconstat( SerialPort ) : 0 );
}

int serial_getc( void )
{
	char c;

	if (!SerialPort)
		return( 0 );
	
	c = Bconin( SerialPort );
	
	/* do some conversions to allow basic editing over serial line (BACKSPACE
	 * and DELETE delete backwards, ^U deletes whole line) */
	if (c == ASC_BS || c == ASC_DEL) 
		return( (SCAN_BS << 16) | ASC_BS );
	else if (c < ' ' && (c != CTRL_U && c != ASC_CR && c != ASC_LF))
		/* don't allow other ctrl or meta characters */
		return( 0 );
	else
		return( c );
}

static void serial_putc( char c )
{
	if (!SerialPort)
		return;

	if (c == '\n')
		/* translate NL to CR-LF */
		Bconout( SerialPort, '\r' );
	Bconout( SerialPort, c );
}


static void serial_puts( const char *p )
{
	if (!SerialPort)
		return;

	while( *p )
		serial_putc( *p++ );
}


/* ------------------------------------------------------------------------- */
/*						 Primitive Graphics Functions						 */

#ifndef NO_GUI

static void text( int x, int y, const char *str )
{
	int dum;
	
	vst_alignment( grh, 0, 5, &dum, &dum );
	v_gtext( grh, x, y, (char *)str );
}

static void texta( int x, int y, int halign, int valign, const char *str )
{
	int dum;
	
	vst_alignment( grh, halign, valign, &dum, &dum );
	v_gtext( grh, x, y, (char *)str );
}

static void texteffect( int effect )
{
	vst_effects( grh, effect );
}

static void textrotate( int deg )
{
	vst_rotation( grh, deg*10 );
}

static void textsize( int dbl )
{
	int dum;
	
	vst_point( grh, dbl ? 20 : lineht == 8 ? 9 : 10, &dum, &dum, &dum, &dum );
}

static void text_extent( const char *str, int *w, int *h )
{
	int ext[8];

	vqt_extent( grh, (char *)str, ext );
	*w = ext[2];
	*h = ext[5];
}

static void writemode( int mode )
{
	vswr_mode( grh, mode );
}

/* draw a box */
static void box( int x, int y, int w, int h, int linew )
{
	int xy[10];

	xy[0] = x-1;		xy[1] = y-1;
	xy[2] = xy[0]+w+1;	xy[3] = xy[1];
	xy[4] = xy[2];		xy[5] = xy[1]+h+1;
	xy[6] = xy[0];		xy[7] = xy[5];
	xy[8] = xy[0];		xy[9] = xy[1];
	v_pline( grh, 5, xy );
	
	while( linew-- > 1 ) {
		xy[0]--; xy[1]--;
		xy[2]++; xy[3]--;
		xy[4]++; xy[5]++;
		xy[6]--; xy[7]++;
		xy[8]--; xy[9]--;
		v_pline( grh, 5, xy );
	}
}

/* copy an icon image to the screen */
static void copyicon( int x, int y )
{
	int colors[2], xy[8];
	MFDB dstMFDB, *icp;
	
	icp = &iconMFDB[icon];
	
	colors[0] = 1;
	colors[1] = 0;
	xy[0] = 0;
	xy[1] = 0;
	xy[2] = icp->fd_w-1;
	xy[3] = icp->fd_h-1;
	xy[4] = x;
	xy[5] = y;
	xy[6] = x+icp->fd_w-1;
	xy[7] = y+icp->fd_h-1;
	dstMFDB.fd_addr = 0;
	vrt_cpyfm( grh, 1, xy, icp, &dstMFDB, colors );
}

/* get mouse position and button status */
static int getmouse( int *x, int *y )
{
	int status;

#ifdef AES
	int dum;
	graf_mkstate( x, y, &status, &dum );
#else	
	vq_mouse( grh, &status, x, y );
#endif
	return( status );
}

/* reverse character cell of cursor */
static void reverse( void )
{
	int xy[4];

	xy[0] = hcmdo + cursor*charw;
	xy[1] = vcmd;
	xy[2] = xy[0]+charw-1;
	xy[3] = xy[1]+lineht-1;
	
	vswr_mode( grh, 3 );
	vr_recfl( grh, xy );
	vswr_mode( grh, 1 );
}

/* clear screen area */
static void clear( int x, int y, int w, int h )
{
	int xy[4];

	xy[0] = x;
	xy[1] = y;
	xy[2] = x+w-1;
	xy[3] = y+h-1;
	
	vsf_color( grh, 0 );
	vr_recfl( grh, xy );
	vsf_color( grh, 1 );
}



/* must implement v_gtext myself, there's a sign extension bug in the
 * library, and additionally some limit on the string length (80 chars
 * or so)
 * Also, can't declare it static, because it's already delcared non-static by
 * <vdibind.h>
 */

short cntrl[7], intin[130], intout[1], ptsin[2], ptsout[1];
struct { short *p1, *p2, *p3, *p4, *p5; } VDIPARS =
  { cntrl, intin, ptsin, intout, ptsout };

int v_gtext( int h, int x, int y, char *s )

{
	int i = 0;
  
	ptsin[0] = x;
	ptsin[1] = y;
	while( (intin[i++] = *(unsigned char *)s++) );
	cntrl[0] = 8;
	cntrl[1] = 1;
	cntrl[2] = 0;
	cntrl[3] = i-1;
	cntrl[4] = 0;
	cntrl[6] = h;
  
	__asm__ __volatile__
		( "movl %0,d1\n\t"
		  "movw #115,d0\n\t"
		  "trap #2"
		  : : "g" (&VDIPARS) );
	return( 0 );
}

#endif /* NO_GUI */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
