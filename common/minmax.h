#ifndef _minmax_h
#define _minmax_h

#define	min(a,b)								\
    ({											\
		typeof(a) __a = (a);					\
		typeof(b) __b = (b);					\
		__a < __b ? __a : __b;					\
	})
	
#define	max(a,b)								\
    ({											\
		typeof(a) __a = (a);					\
		typeof(b) __b = (b);					\
		__a > __b ? __a : __b;					\
	})
	
#endif  /* _minmax_h */

