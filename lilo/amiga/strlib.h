#ifndef _strlib_h
#define _strlib_h

extern size_t strlen(const char * s);
extern int strcmp(const char * cs,const char * ct);
extern int strncmp(const char * cs,const char * ct,size_t count);
extern char * strcpy(char * dest,const char *src);
extern char * strncpy(char * dest,const char *src,size_t count);
extern char * strcat(char * dest, const char * src);
extern char * strncat(char *dest, const char *src, size_t count);
extern void * memset(void * s,int c,size_t count);
extern void * memcpy(void * dest,const void *src,size_t count);
extern int memcmp(const void * cs,const void * ct,size_t count);

extern unsigned long strtoul(const char *cp,char **endp,unsigned int base);

/*
 * NOTE! This ctype does not handle EOF like the standarc C
 * library is required to.
 */

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define islower(c)	((__ismask(c)&(_L)) != 0)
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)

static inline unsigned char toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#endif  /* _strlib_h */

