/*
 *  Amiga Linux/m68k LILO -- Machine-specific parser rules
 *
 *  © Copyright 1995,97 by Geert Uytterhoeven, Roman Hodek
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: mach-rules.y,v 1.2 1997-08-12 21:51:06 rnhodek Exp $
 * 
 * $Log: mach-rules.y,v $
 * Revision 1.2  1997-08-12 21:51:06  rnhodek
 * Written last missing parts of Atari lilo and made everything compile
 *
 * Revision 1.1  1997/08/12 15:27:05  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */


bootos    : "amiga"	{ $$ = BOS_AMIGA; }
	  | "linux"	{ $$ = BOS_LINUX; }
	  | "netbsd"	{ $$ = BOS_NETBSD; }
	  ;

altdev    : "altdev" STRING NUMBER
	    {
		if (Config.AltDeviceName)
		    Redefinition((char *)$1);
		Config.AltDeviceName = (const char *)$2;
		Config.AltDeviceUnit = CopyLong($3);
		if (strlen(Config.AltDeviceName) >= MAXALTDEVSIZE)
		    conferror("Alternate boot device name is too long");
	    }
	  ;

aux	  : "aux" opt_bool
	    {
		if (Config.Options.Aux)
		    Redefinition((char *)$1);
		Config.Options.Aux = CopyLong($2);
	    }
	  ;

baud      : "baud" NUMBER
	    {
		if (Config.Options.Baud)
		    Redefinition((char *)$1);
		Config.Options.Baud = CopyLong($2);
	    }
	  ;

kickrange : "kickrange" NUMBER NUMBER
	    {
		if (Config.Options.KickRange)
		    Redefinition((char *)$1);
		if ($2 > 65535 || $3 > 65535 || $3 < $2)
		    conferror("Invalid Kickstart version range");
		Config.Options.KickRange = CopyShort2($2, $3);
	    }
	  ;

g_chipram   : "chipram" NUMBER
	    {
		if (Config.Options.ChipSize)
		    Redefinition((char *)$1);
		Config.Options.ChipSize = CopyLong($2);
	    }
	  ;

g_fastram   : "fastram" NUMBER NUMBER
	    {
		int i;

		for (i = 0; i < arraysize(Config.Options.FastChunks); i++)
		    if (!Config.Options.FastChunks[i])
			break;
		if (i == arraysize(Config.Options.FastChunks))
		    conferror("Too many Fast RAM Chunks");
		Config.Options.FastChunks[i] = CopyLong2($2, $3);
	    }

g_model     : "model" amimodel
	    {
		if (Config.Options.Model)
		    Redefinition((char *)$1);
		Config.Options.Model = CopyLong($2);
	    }
	  ;

amimodel    : "A500"	{ $$ = AMI_500; }
	    | "A500+"	{ $$ = AMI_500PLUS; }
	    | "A600"	{ $$ = AMI_600; }
	    | "A1000"	{ $$ = AMI_1000; }
	    | "A1200"	{ $$ = AMI_1200; }
	    | "A2000"	{ $$ = AMI_2000; }
	    | "A2500"	{ $$ = AMI_2500; }
	    | "A3000"	{ $$ = AMI_3000; }
	    | "A3000T"	{ $$ = AMI_3000T; }
	    | "A3000+"	{ $$ = AMI_3000PLUS; }
	    | "A4000"	{ $$ = AMI_4000; }
	    | "A4000T"	{ $$ = AMI_4000T; }
	    | "CDTV"	{ $$ = AMI_CDTV; }
	    | "CD32"	{ $$ = AMI_CD32; }
	    | "Draco"	{ $$ = AMI_DRACO; }
	    | NUMBER
	    ;

g_processor : "processor" NUMBER
	    {
		if (Config.Options.Processor)
		    Redefinition((char *)$1);
		Config.Options.Processor = CopyLong($2);
	    }
	  ;

video	    : "video" STRING
	    {
		if (Config.Options.Video)
		    Redefinition((char *)$1);
		Config.Options.Video = (const char *)$2;
	    }
	  ;

serial	    : "serial" STRING
	    {
		if (Config.Options.Serial)
		    Redefinition((char *)$1);
		Config.Options.Serial = (const char *)$2;
	    }
	  ;

bootrec   : "bootrec" STRING bootopts "endrec"
	    {
		BootRecord.Label = (const char *)$2;
		if (BootRecord.OSType && *BootRecord.OSType == BOS_AMIGA) {
		    if (BootRecord.Kernel)
			conferror("You can't specify a kernel image for "
				  "AmigaOS");
		    if (BootRecord.Args)
			conferror("You can't specify arguments for AmigaOS");
		    if (BootRecord.Ramdisk)
			conferror("You can't specify a ramdisk image for "
				  "AmigaOS");
		} else if (!BootRecord.Kernel)
		    conferror("No kernel image specified");
		if (!AddBootRecord(&BootRecord))
		    conferror("Duplicate boot record label or alias");
		ClearBootRecord();
	    }
	  ;

chipram   : "chipram" NUMBER
	    {
		if (BootRecord.ChipSize)
		    Redefinition((char *)$1);
		BootRecord.ChipSize = CopyLong($2);
	    }
	  ;

fastram   : "fastram" NUMBER NUMBER
	    {
		int i;

		for (i = 0; i < arraysize(BootRecord.FastChunks); i++)
		    if (!BootRecord.FastChunks[i])
			break;
		if (i == arraysize(BootRecord.FastChunks))
		    conferror("Too many Fast RAM Chunks");
		BootRecord.FastChunks[i] = CopyLong2($2, $3);
	    }

model     : "model" amimodel
	    {
		if (BootRecord.Model)
		    Redefinition((char *)$1);
		BootRecord.Model = CopyLong($2);
	    }
	  ;

processor : "processor" NUMBER
	    {
		if (BootRecord.Processor)
		    Redefinition((char *)$1);
		BootRecord.Processor = CopyLong($2);
	    }
	  ;

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
