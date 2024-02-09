#include <stdio.h>
#include <asm/termios.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main( int argc, char ** argv )
{
	if( argc != 3 )
	{
		fprintf( stderr, "Usage: [port] [baud]\n" );
		exit( -1 );
	}

	int fd = open( argv[1], O_RDWR );

	struct termios2 tio;

	int r1 = ioctl(fd, TCGETS2, &tio);
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER;
	tio.c_ispeed = atoi( argv[2] );
	tio.c_ospeed = atoi( argv[2] );
	int r2 = ioctl(fd, TCSETS2, &tio);
	printf( "%d/%d\n", r1, r2 );
}
