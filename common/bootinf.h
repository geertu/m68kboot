#ifndef _bootinf_h
#define _bootinf_h


/***************************** Prototypes *****************************/

int check_bootinfo_version( char *memptr, unsigned int mach_type, unsigned int
                            my_version, unsigned int compat_version );
int create_bootinfo( void);
int add_bi_record( u_short tag, u_short size, const void *data);
int add_bi_string( u_short tag, const u_char *s);
int create_compat_bootinfo( void);

/************************* End of Prototypes **************************/


#endif  /* _bootinf_h */

