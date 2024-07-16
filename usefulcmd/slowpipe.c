#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv )
{
	float wtime = 500;
	float ctime = 10;

	int iwt, ict;

	int i;
	int c;
	for( i = 1; i < argc; i++ )
	{
		if( strcmp( argv[i], "-w" ) == 0 )
		{
			i++;
			wtime = atof( argv[i] );
		}
		else if( strcmp( argv[i], "-c" ) == 0 )
		{
			i++;
			ctime = atof( argv[i] );
		}
		else
		{
			fprintf( stderr, "Error: usage: slowpipe [-w wait before send time in ms] [-c wait per char time in ms]\n" );
			return -1;
		}
	}

	iwt = wtime * 1000;
	ict = ctime * 1000;

	usleep( iwt );

	while( ( c = getchar() ) != EOF )
	{
		usleep( ict );
		putchar( c );
		fflush( stdout );
	}

	return 0;
}
