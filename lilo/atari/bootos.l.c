#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct bf {
	unsigned char flag;
	char *name;
} flagtab[] = {
	{ 0x80, "GEMDOS" },
	{ 0x80, "TOS" },
	{ 0x40, "Atari System V" },
	{ 0x40, "ASV" },
	{ 0x20, "NetBSD" },
	{ 0x10, "Linux" },
	{ 0x00, "unspecified" },
	{ 0, NULL }
};

#define	SCU_GPIP1	  0xffff8e09


char *find_flag( unsigned char flag )
{
	struct bf *p;
	static char buf[32];

	for( p = flagtab; p->name; ++p ) {
		if (p->flag == flag)
			return( p->name );
	}
	sprintf( buf, "Unknown (0x%02x)", flag );
	return( buf );
}


int find_os( const char *name )
{
	int len = strlen(name);
	int matches = 0;
	struct bf *p, *match = NULL;

	for( p = flagtab; p->name; ++p ) {
		if (strncasecmp( p->name, name, len ) == 0) {
			match = p;
			++matches;
		}
	}
	if (matches == 1)
		return( match->flag );
	if (matches > 1)
		fprintf( stderr, "Ambigous abbreviation %s\n", name );
	return( -1 );
}


void list_names( void )
{
	struct bf *p;

	for( p = flagtab; p->name; ++p ) {
		printf( "  %s (0x%02x)\n", p->name, p->flag );
	}
}


void help( void )
{
	printf( "Usage: bootos [-t] [-q] [new-os-to-boot]\n"
			"  -t: temporary setting, only valid till power off\n"
			"  -q: be quiet\n" );
}


int main( int argc, char *argv[] )
{
	int ch, fd, open_mode, pos, rv;
	int temporary = 0;
	int quiet = 0;
	int have_newos = 0;
	unsigned char oldos, newos;
	char *file;
	
	if (argc > 1 && strcmp( argv[1], "--help" ) == 0) {
		help();
		exit( 0 );
	}

    while( (ch = getopt( argc, argv, "tq" )) != EOF ) {
		switch( ch ) {
		  case 't':
			temporary = 1;
			break;
		  case 'q':
			quiet = 1;
			break;
		  case '?':
		  case 'h':
			help();
			exit( 0 );
		  default:
			fprintf( stderr, "Unknown option %c\n", ch );
			help();
			exit( 1 );
		}
	}
	
	if (optind < argc) {
		int x = find_os( argv[optind] );
		if (x < 0) {
			fprintf( stderr, "Unknown OS name %s\n", argv[optind] );
			fprintf( stderr, "Available names are:\n" );
			list_names();
			exit( 1 );
		}
		newos = (unsigned char)x;
		have_newos = 1;
	}

	open_mode = have_newos ? O_RDWR : O_RDONLY;
	if (temporary) {
		file = "/dev/kmem";
		pos = SCU_GPIP1;
	}
	else {
		file = "/dev/nvram";
		pos = 1;
	}
	
	if ((fd = open( file, open_mode )) < 0) {
		perror( file );
		exit( 1 );
	}
	if (lseek( fd, pos, SEEK_SET ) != pos) {
		perror( "seek" );
		exit( 1 );
	}
	if (read( fd, &oldos, 1 ) != 1) {
		perror( "read" );
		exit( 1 );
	}

	if (!quiet) {
		if (have_newos)
			printf( "Old boot OS: " );
		printf( "%s\n", find_flag( oldos & 0xf8 ));
	}
	
	if (have_newos) {
		if (lseek( fd, pos, SEEK_SET ) != pos) {
			perror( "seek" );
			exit( 1 );
		}
		newos = (newos & 0xf8) | (oldos & 0x07);
		if ((rv = write( fd, &newos, 1 )) != 1) {
			if (rv == 0)
				fprintf( stderr, "Sorry, kernel doesn't support writing to "
						 "/dev/kmem beyond high_memory :-(\n" );
			else
				perror( "write" );
			exit( 1 );
		}
		if (!quiet) {
			printf( "New boot OS: %s\n", find_flag( newos & 0xf8 ));
		}
	}
	close( fd );

	return( 0 );
}
