/*
 *  Atari Linux/m68k LILO -- Machine-specific parser rules
 *
 *  © Copyright 1997 by Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: mach-rules.y,v 1.8 1998-04-08 09:32:35 schwab Exp $
 * 
 * $Log: mach-rules.y,v $
 * Revision 1.8  1998-04-08 09:32:35  schwab
 * Fix spelling "loadto-st-ram" -> "load-to-st-ram".
 *
 * Revision 1.7  1998/03/10 10:26:34  rnhodek
 * New boot record option "restricted".
 *
 * Revision 1.6  1998/03/06 09:50:25  rnhodek
 * New option skip-on-keys.
 *
 * Revision 1.5  1998/03/04 09:18:03  rnhodek
 * New config var array ProgCache[] as option to 'exec'.
 * WorkDir and ProgCache also for record-specific exec's.
 *
 * Revision 1.4  1998/02/26 10:20:34  rnhodek
 * New config vars WorkDir, Environ, and BootDrv (global)
 * Remove some const warnings.
 *
 * Revision 1.3  1997/09/19 09:06:59  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.2  1997/08/23 23:09:50  rnhodek
 * New parameter 'set_bootdev' to parse_device
 *
 * Revision 1.1  1997/08/12 15:27:10  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

bootos: "tos"			{ $$ = BOS_TOS; }
	  | "linux"			{ $$ = BOS_LINUX; }
	  | "bootsector"	{ $$ = BOS_BOOTB; }
	  ;

delay: "delay" NUMBER
	{
		if (Config.Options.Delay)
		    Redefinition((char *)$1);
		Config.Options.Delay = CopyLong($2);
	};

nogui: "nogui" opt_bool
	{
		if (Config.Options.NoGUI)
		    Redefinition((char *)$1);
		Config.Options.NoGUI = CopyLong($2);
	};

serial: "serial" serialdev NUMBER SERIALPARAM opt_flow
	{
		int speedcode;
		if (Config.Options.Serial)
		    Redefinition((char *)$1);
		if ((speedcode = SerialSpeedCode($3)) < 0)
			conferror( "Invalid serial speed" );
		Config.Options.Serial = CopyLong($2);
		Config.Options.SerialBaud = CopyLong(speedcode);
		Config.Options.SerialParam = (const char *)$4;
		Config.Options.SerialFlow = CopyLong($5);
	};

serialdev: "default"	{ $$ = 1; }
		 | "modem1"		{ $$ = 6; }
		 | "modem2"		{ $$ = 7; }
		 | "serial1"	{ $$ = 8; }
		 | "serial2"	{ $$ = 9; }
		 | NUMBER;

opt_flow: "none"		{ $$ = 0; }
		| "xon"			{ $$ = 1; }
		| "rtscts"		{ $$ = 2; }
		| "xon-rtscts"	{ $$ = 3; }
		| /* empty */	{ $$ = 0; };

nobing: "no-bing" opt_bool
	{
		if (Config.Options.NoBing)
		    Redefinition((char *)$1);
		Config.Options.NoBing = CopyLong($2);
	};

videores: "resolution" vresol
	{
		if (Config.Options.VideoRes)
		    Redefinition((char *)$1);
		Config.Options.VideoRes = CopyLong($2);
	};

vresol: "st-low"	{ $$ = 0; }
	  | "st-mid"	{ $$ = 1; }
	  | "st-high"	{ $$ = 2; }
	  | "tt-low"	{ $$ = 7; }
	  | "tt-mid"	{ $$ = 4; }
	  | "tt-high"	{ $$ = 6; }
      | NUMBER;

mch_cookie: "machine" machcode
	{
		if (Config.Options.MCH_cookie)
		    Redefinition((char *)$1);
		Config.Options.MCH_cookie = CopyLong($2);
	};

machcode: "st"				{ $$ = 0x00000000; }
		| "ste"				{ $$ = 0x00010000; }
		| "megaste"			{ $$ = 0x00010010; }
		| "tt"				{ $$ = 0x00020000; }
		| "falcon"			{ $$ = 0x00030000; }
		| NUMBER            { $$ = $0 << 16; }
        | NUMBER '/' NUMBER { $$ = ($0 << 16) | ($3 & 0xffff); }
		;

cpu_cookie: "cpu" NUMBER
	{
		if (Config.Options.CPU_cookie)
		    Redefinition((char *)$1);
		if ($2 / 1000 == 68)
			$2 -= 68000;
		if ($2 != 20 && $2 != 30 && $2 != 40 && $2 != 60)
			conferror( "Bad CPU type" );
		Config.Options.CPU_cookie = CopyLong($2);
	};

fpu_cookie: "fpu" fpucode
	{
		unsigned int code;
		if (Config.Options.FPU_cookie)
		    Redefinition((char *)$1);
		switch( $2 ) {
		  case 0:		code = 0; break;
		  case 68881:	code = 0x00040000; break;
		  case 68882:	code = 0x00060000; break;
		  case 68040:	code = 0x00080000; break;
		  default:		printf( "%u", $2 ); conferror( "Bad FPU type" );
		}
		Config.Options.FPU_cookie = CopyLong(code);
	};

fpucode: "none"		{ $$ = 0; }
	   | "intern"	{ $$ = 68040; }
	   | NUMBER
	   ;

g_tmpmnt: "mount" STRING "on" TOSDRIVESPEC opt_rw
	{
		int i, j, dev;
		unsigned long start;

		parse_device( (char *)$2, &dev, &start, 0, ANY_FLOPPY, 0 );
		for (i = 0; i < arraysize(Config.Options.TmpMnt); i++)
		    if (!Config.Options.TmpMnt[i])
			break;
		if (i == arraysize(Config.Options.TmpMnt))
		    conferror("Too many mounts");
		j = AddTmpMnt( dev, start, $4, $5 );
		Config.Options.TmpMnt[i] = CopyLong(j);
	};

opt_rw: "ro"		{ $$ = 0; }
	  | "rw"		{ $$ = 1; }
	  | /* empty */ { $$ = 0; } ;

g_execprog: "exec" STRING opt_workdir opt_cache
	{
		int i;
		
		for (i = 0; i < arraysize(Config.Options.ExecProg); i++)
		    if (!Config.Options.ExecProg[i])
				break;
		if (i == arraysize(Config.Options.ExecProg))
		    conferror("Too many programs to execute");
		Config.Options.ExecProg[i]  = (const char *)$2;
		Config.Options.WorkDir[i]   = (const char *)$3;
		Config.Options.ProgCache[i] = CopyLong($4);
	};

opt_workdir: "chdir" STRING	{ $$ = $2; }
		   | /* empty */	{ $$ = 0; } ;

opt_cache: "no-cache"		{ $$ = 0; }
		 | /* empty */		{ $$ = 1; };

g_bootdrv: "boot-drive" TOSDRIVESPEC
	{
		if (Config.Options.BootDrv)
		    Redefinition((char *)$1);
		Config.Options.BootDrv = CopyLong($2);
	};

setenv: "setenv" STRING
	{
		int i;

		if (!strchr( (const char *)$2, '=' ))
			conferror("Environment setting doesn't contain a '='");
		for (i = 0; i < arraysize(Config.Options.Environ); i++)
		    if (!Config.Options.Environ[i])
				break;
		if (i == arraysize(Config.Options.Environ))
		    conferror("Too many environment variables");
		Config.Options.Environ[i] = (const char *)$2;
	};

skiponkeys: "skip-on-keys" STRING
	{
		Config.SkipOnKeys = CopyLong( ParseKeyMask( (const char *)$2 ));
	};

bootrec: "bootrec" STRING bootopts "endrec"
    {
		BootRecord.Label = (const char *)$2;
		EXCLUSIVE_ATTR( 1, BOS_LINUX, "Linux", Kernel, "kernel image" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", Args, "argument" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", Ramdisk, "RAM disk" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", TmpMnt[0], "mount" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", ExecProg[0], "program execution" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", IgnoreTTRam, "ignore-tt-ram" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", LoadToSTRam, "load-to-st-ram" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", ForceSTRam, "force-st-ram-size" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", ForceTTRam, "force-tt-ram-size" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", ExtraMemStart, "extra-mem" );
		EXCLUSIVE_ATTR( 0, BOS_LINUX, "Linux", ExtraMemSize, "" );
		EXCLUSIVE_ATTR( 1, BOS_TOS, "TOS", TOSDriver, "TOS driver" );
		EXCLUSIVE_ATTR( 0, BOS_TOS, "TOS", BootDrv, "boot drive" );
		if (!BootRecord.OSType) {
			if (BootRecord.BootDev) {
				if (BootRecord.TOSDriver)
					BootRecord.OSType = CopyLong( BOS_TOS );
				else
					BootRecord.OSType = CopyLong( BOS_BOOTB );
			}
			else
				conferror( "OS type undefined for this record" );
		}
		if ((*BootRecord.OSType == BOS_TOS || *BootRecord.OSType == BOS_BOOTB) &&
			(!BootRecord.BootDev || !BootRecord.BootSec)) 
			conferror( "Boot device/partition not defined" );
		if (!AddBootRecord(&BootRecord))
		    conferror( "Duplicate boot record label or alias" );
		ClearBootRecord();
	};

restricted: "restricted" opt_bool
	{
		if (BootRecord.Restricted)
		    Redefinition((char *)$1);
		BootRecord.Restricted = CopyLong($2);
	};

ignorettram: "ignore-tt-ram" opt_bool
	{
		if (BootRecord.IgnoreTTRam)
		    Redefinition((char *)$1);
		BootRecord.IgnoreTTRam = CopyLong($2);
	};

loadtostram: "load-to-st-ram" opt_bool
	{
		if (BootRecord.LoadToSTRam)
		    Redefinition((char *)$1);
		BootRecord.LoadToSTRam = CopyLong($2);
	};

forcestram: "force-st-ram-size" NUMBER
	{
		if (BootRecord.ForceSTRam)
		    Redefinition((char *)$1);
		BootRecord.ForceSTRam = CopyLong($2);
	};

forcettram: "force-tt-ram-size" NUMBER
	{
		if (BootRecord.ForceTTRam)
		    Redefinition((char *)$1);
		BootRecord.ForceTTRam = CopyLong($2);
	};

extramem: "extra-mem" NUMBER "at" NUMBER
	{
		if (BootRecord.ExtraMemStart || BootRecord.ExtraMemSize)
		    Redefinition((char *)$1);
		BootRecord.ExtraMemStart = CopyLong($2);
		BootRecord.ExtraMemSize = CopyLong($4);
	};

tmpmnt: "mount" STRING "on" TOSDRIVESPEC opt_rw
	{
		int i, j, dev;
		unsigned long start;

		parse_device( (char *)$2, &dev, &start, 0, ANY_FLOPPY, 0 );
		for (i = 0; i < arraysize(BootRecord.TmpMnt); i++)
		    if (!BootRecord.TmpMnt[i])
			break;
		if (i == arraysize(BootRecord.TmpMnt))
		    conferror("Too many mounts");
		j = AddTmpMnt( dev, start, $4, $5 );
		BootRecord.TmpMnt[i] = CopyLong(j);
	};

execprog: "exec" STRING opt_workdir opt_cache
	{
		int i;
		
		for (i = 0; i < arraysize(BootRecord.ExecProg); i++)
		    if (!BootRecord.ExecProg[i])
			break;
		if (i == arraysize(BootRecord.ExecProg))
		    conferror("Too many programs to execute");
		BootRecord.ExecProg[i] = (const char *)$2;
		BootRecord.WorkDir[i]   = (const char *)$3;
		BootRecord.ProgCache[i] = CopyLong($4);
	};


tosdriver: "driver" STRING
    {
		if (BootRecord.TOSDriver)
		    Redefinition((char *)$1);
		BootRecord.TOSDriver = (const char *)$2;
    };

bootdrv: "boot-drive" TOSDRIVESPEC
	{
		if (BootRecord.BootDrv)
		    Redefinition((char *)$1);
		BootRecord.BootDrv = CopyLong($2);
	};
		
bootpart: "partition" STRING
	{
		int dev;
		unsigned long start;
	
		if (BootRecord.BootDev || BootRecord.BootSec)
		    Redefinition((char *)$1);
		parse_device( (char *)$2, &dev, &start, 0, NO_FLOPPY, 0 );
		BootRecord.BootDev = CopyLong(dev);
		BootRecord.BootSec = CopyLong(start);
	}

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
