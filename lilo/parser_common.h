/*
 *  Linux/m68k LILO -- Configuration File Parsing
 *
 *  © Copyright 1995 by Geert Uytterhoeven
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING for more details.
 * 
 * $Id: parser_common.h,v 1.1 1997-08-12 15:26:57 rnhodek Exp $
 * 
 * $Log: parser_common.h,v $
 * Revision 1.1  1997-08-12 15:26:57  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 * 
 */

#ifndef _parser_common_h
#define _parser_common_h

#include "config.h"

#define CONFIG_COMMON							\
    struct BootOptions Options;					\
    struct BootRecord *Records;					\
    struct FileDef *Files;

extern struct Config Config;
extern const char *ConfigFile;
extern FILE *confin;

extern int confparse(void);
extern void Die(const char *fmt, ...) __attribute__
    ((noreturn, format (printf, 1, 2)));
extern void Error_NoMemory(void) __attribute__ ((noreturn));
extern int AddBootRecord(const struct BootRecord *record);
extern void AddFileDef(const char *path);

#endif  /* _parser_common_h */

