#include <stdio.h>

int main( int argc, char ** argv )
{
	unsigned long startbyte;
	unsigned long endbyte;
	unsigned long i;
	unsigned long col = 0;

	if( argc > 3 || ( argc > 1 && argv[1][0] == '-' ) )
	{
		fprintf( stderr, "Usage: [tool] [start byte] [end byte]\n" );
		return -1;
	}

	if( argc > 1 )
		startbyte = atoi( argv[1] );
	else
		startbyte = 0;

	if( argc > 2 )
		endbyte = atoi( argv[2] );
	else
		endbyte = 0xffffffff;

	for( i = 0; i < startbyte && !feof( stdin ); i++ )
	{
		getchar();
	}

	printf( "\t" );

	for( ; i < endbyte && !feof( stdin ); i++ )
	{
		printf( "0x%02x, ", getchar() );
		if( col++ == 15 )
		{
			col = 0;
			printf( "\n\t" );
		}
	}
	printf( "\n" );

	return 0;
}
