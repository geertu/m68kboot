/* menu.h -- Definitions for menu.c
 *
 * Copyright (C) 1997 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This program is free software.  You can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation: either version 2 or
 * (at your option) any later version.
 * 
 * $Id: menu.h,v 1.4 1998-03-10 10:29:58 rnhodek Exp $
 * 
 * $Log: menu.h,v $
 * Revision 1.4  1998-03-10 10:29:58  rnhodek
 * Make serial_instat and serial_getc global functions.
 * waitkey(): New function.
 *
 * Revision 1.3  1998/03/02 13:12:39  rnhodek
 * New function goto_last_line().
 *
 * Revision 1.2  1998/02/25 10:37:46  rnhodek
 * New argument 'doprompt' to read_line().
 *
 * Revision 1.1  1997/08/12 15:27:11  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _menu_h
#define _menu_h

/***************************** Prototypes *****************************/

char *boot_menu( const char *dflt_label );
void menu_error( const char *str );
char *get_password( void );
void graf_init( const unsigned long *video_res );
void graf_deinit( void );
void goto_last_line( void );
char *read_line( int dotimeout, int doprompt );
int waitkey( void );
void cprintf( const char *format, ... )
	__attribute__((format(printf,1,2)));
int serial_instat( void );
int serial_getc( void );
int v_gtext( int h, int x, int y, char *s );

/************************* End of Prototypes **************************/

#endif  /* _menu_h */
