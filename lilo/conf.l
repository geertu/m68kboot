/*
 *  Amiga Linux/m68k Loader -- Configuration File Lexical Analyzer
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: conf.l,v 1.1 1997-08-12 15:26:55 rnhodek Exp $
 * 
 * $Log: conf.l,v $
 * Revision 1.1  1997-08-12 15:26:55  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


%{

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/amigahw.h>

#include "linuxboot.h"
#include "config.h"
#include "conf.tab.l.h"
#include "parser.h"


extern int FindToken( const char *s );

int line = 1;

void conferror(const char *s)
{
    Die("%s:%d: %s\n", ConfigFile, line, s);
}


int confwrap(void)
{
    return(1);
}

static const char *CopyString(const char *s)
{
    int len, size, i;
    char *s2;

    len = strlen(s)-2;
    /* Round up to a multiple of 4 bytes */
    size = (len+4)&~3;
    if (!(s2 = malloc(size)))
	Error_NoMemory();
    strncpy(s2, s+1, len);
    for (i = len; i < size; i++)
	s2[i] = '\0';
    return(s2);
}


%}

keyword	[a-zA-Z][a-zA-Z0-9+-]*
decnum	[1-9][0-9]*
hexnum	0[xX][0-9a-fA-f]+
octnum	0[0-7]*
number	{decnum}|{hexnum}|{octnum}
string	\"[^\"\n]*\"
comment	\#([^\n]*)
space	[ \t]+
junk	.

%%

{keyword}   {
		return(FindToken(conftext));
	    }

[5678][neoNEO](1|1.5|2) {
		if (!(conflval = (int)strdup(conftext)))
		    Error_NoMemory();
		return( SERIALPARAM );
	    }

[a-zA-Z]:   {
		conflval = toupper(conftext[0] - 'A');
		return( TOSDRIVESPEC );
	    }

{number}[kKmM] {
		char *endp;
		conflval = strtoul(conftext, &endp, 0);
		if (tolower(*endp) == 'k')
			conflval *= 1024;
		else if (tolower(*endp) == 'm')
			conflval *= 1024*1024;
		return(NUMBER);
	    }
		
{number}    {
		conflval = strtoul(conftext, NULL, 0);
		return(NUMBER);
	    }

{string}    {
		conflval = (int)CopyString(conftext);
		return(STRING);
	    }

{comment}$  break;

{space}     break;

\n	    {
		line++;
		break;
	    }

{junk}	    {
		Die("%s:%d: Invalid token `%s'\n", ConfigFile, line, conftext);
	    }

%%

/* Local Variables: */
/* mode: c          */
/* tab-width: 8     */
/* End:             */
