#ifndef _version_h
#define _version_h

#define	VERSION			"6.0snapshot " __DATE__
#ifdef USE_BOOTP
#define	WITH_BOOTP		" (with BOOTP)"
#else
#define	WITH_BOOTP		""
#endif

#endif  /* _version_h */

