/*
 *  Linux/m68k Loader -- Configuration File Parser
 *
 *  (©) Copyright 1995-97 by Geert Uytterhoeven, Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: conf.y,v 1.3 2004-10-01 10:06:21 geert Exp $
 * 
 * $Log: conf.y,v $
 * Revision 1.3  2004-10-01 10:06:21  geert
 * Recent versions of bison need YYERROR_VERBOSE and YYPRINT to be defined to make
 * yytname and yytoknum visible
 *
 * Revision 1.2  1998/03/10 10:22:13  rnhodek
 * New option "message".
 *
 * Revision 1.1  1997/08/12 15:26:56  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


%{

@include <stdio.h>
@include <stdlib.h>
@include <ctype.h>
@include <string.h>
@include <sys/types.h>

@include "linuxboot.h"
@include "config.h"
@include "parser.h"

@define YYERROR_VERBOSE
@define YYPRINT

extern int conflex(void);
extern void conferror(const char *s) __attribute__ ((noreturn));
extern int line;
extern struct Config Config;	/* forward decl */

static struct BootRecord BootRecord;

static void ClearBootRecord()
{
    memset(&BootRecord, 0, sizeof(BootRecord));
}

static void Redefinition(const char *name)
{
    printf("%s:%d: Redefinition of `%s'\n", ConfigFile, line, name);
}

static const u_long *CopyLong(u_long data)
{
    u_long *p;

    if (!(p = malloc(sizeof(u_long))))
	Error_NoMemory();
    *p = data;
    return(p);
}

#include "mach-parsefuncs.c"

struct Config Config = {
	MACH_CONFIG_INIT
};

%}

%start file

%token_table
%token FIRSTTOKEN NUMBER STRING SERIALPARAM TOSDRIVESPEC

%%

boolean   : "true"  { $$ = 1; }
	  | "false" { $$ = 0; }
	  ;

opt_bool  : /* empty */ { $$ = 1; }
	  | boolean
	  ;

file	  : header bootrecs filedefs
	  ;

header	  : "header" hdropts "endheader"
	    {
		if (!Config.BootDevice)
		    conferror("No boot device specified");
		ClearBootRecord();
	    }
	  ;

hdropts	  : /* empty */
	  | hdropts bootdev
	  | hdropts default
	  | hdropts auto
	  | hdropts timeout
	  | hdropts mrpasswd
	  | hdropts debug
	  | hdropts prompt
	  | hdropts message
	  MACH_HDROPTS
	  ;

type      : "type" bootos
	    {
		if (BootRecord.OSType)
		    Redefinition((char *)$1);
		BootRecord.OSType = CopyLong($2);
	    }
	  ;

bootdev	  : "bootdev" STRING
	    {
		if (Config.BootDevice)
		    Redefinition((char *)$1);
		Config.BootDevice = (const char *)$2;
	    }
	  ;

default   : "default" STRING
	    {
		if (Config.Options.Default)
		    Redefinition((char *)$1);
		Config.Options.Default = (const char *)$2;
	    }
	  ;

auto      : "auto" opt_bool
	    {
		if (Config.Options.Auto)
		    Redefinition((char *)$1);
		Config.Options.Auto = CopyLong($2);
	    }
	  ;

timeout   : "timeout" NUMBER
	    {
		if (Config.Options.TimeOut)
		    Redefinition((char *)$1);
		Config.Options.TimeOut = CopyLong($2);
	    }
	  ;

mrpasswd  : "password" STRING
	    {
		if (Config.Options.MasterPassword)
		    Redefinition((char *)$1);
		Config.Options.MasterPassword = (const char *)$2;
	    }
	  ;

debug     : "debug" opt_bool
	    {
		if (Config.Options.Debug)
		    Redefinition((char *)$1);
		Config.Options.Debug = CopyLong($2);
	    }
	  ;

prompt	  : "prompt" STRING
	    {
		if (Config.Options.Prompt)
		    Redefinition((char *)$1);
		Config.Options.Prompt = (const char *)$2;
	    }
	  ;

message	  : "message" STRING
	    {
		char *message = (char *)$2;

		if (Config.Options.Message)
		    Redefinition((char *)$1);
		/* interpret as file name if starts with a '/' */
		if (message[0] == '/') {
		    const char *name = CreateFileName( message );
		    ReadFile( name, (void **)&message );
		}
		else {
		    message = strdup( message );
		    FixSpecialChars( message );
		}
		Config.Options.Message = message;
	    }
	  ;

bootrecs  : bootrec
	  | bootrecs bootrec
	  ;

bootopts  : /* empty */
	  | bootopts image
	  | bootopts args
	  | bootopts passwd
	  | bootopts alias
	  | bootopts ramdisk
	  MACH_BOOTOPTS
	  ;

image     : "image" STRING
	    {
		if (BootRecord.Kernel)
		    Redefinition((char *)$1);
		BootRecord.Kernel = (const char *)$2;
		AddFileDef(BootRecord.Kernel);
	    }
	  ;

args      : "args" STRING
	    {
		if (BootRecord.Args)
		    Redefinition((char *)$1);
		BootRecord.Args = (const char *)$2;
		if (strlen(BootRecord.Args) >= CL_SIZE)
		    conferror("Boot argument is too long");
	    }
	  ;

passwd    : "password" STRING
	    {
		if (BootRecord.Password)
		    Redefinition((char *)$1);
		BootRecord.Password = (const char *)$2;
	    }
	  ;

alias	  : "alias" STRING
	    {
		if (BootRecord.Alias)
		    Redefinition((char *)$1);
		BootRecord.Alias = (const char *)$2;
	    }
	  ;

ramdisk	  : "ramdisk" STRING
	    {
		if (BootRecord.Ramdisk)
		    Redefinition((char *)$1);
		BootRecord.Ramdisk = (const char *)$2;
		AddFileDef(BootRecord.Ramdisk);
	    }
	  ;

filedefs  : /* empty */
	  | filedefs filedef
	  ;

filedef	  : "file" STRING
	    {
		AddFileDef((const char *)$2);
	    }
	  ;

#include "mach-rules.y"

%%

    /*
     * Look up a string in bison's token table
     */
int FindToken(const char *s)
{
    int i;
    int len = strlen(s);

    for( i = 0; i < YYNTOKENS; i++ ) {
	if (yytname[i] &&
	    yytname[i][0] == '"' &&
	    strncasecmp( yytname[i]+1, s, len ) == 0 &&
	    yytname[i][len+1] == '"' &&
	    yytname[i][len+2] == 0) {
	    return( yytoknum[i] );
	}
    }
    Die( "%s:%d: Unknown keyword `%s'\n", ConfigFile, line, s );
    return( 0 );
}


/* Local Variables: */
/* tab-width: 8     */
/* End:             */
