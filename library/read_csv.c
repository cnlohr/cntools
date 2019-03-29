#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *** lines = 0;

int LoadFile( const char * filename )
{
	char * filedata;
	FILE * f = fopen( filename, "rb" );
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	filedata = malloc( len + 1 );
	fread( filedata, len, 1, f );
	filedata[len] = 0;
	fclose( f );
	
	int nrlines = 1;
	int nrsegs = 1;
	int i;
	lines = malloc( nrlines * sizeof( char ** ) );
	char *** thisline = &lines[nrlines-1];
	*thisline = malloc( nrsegs * sizeof( char* ) );
	for( i = 0; i < len; i++ )
	{
		char c = filedata[i];
		if( c == 13 ) filedata[i] = 0;
		else if( c == '\n' )
		{
			filedata[i] = 0;
			nrlines++;
			if( nrlines % 100000 == 0 ) printf( "%d\n", nrlines );
			nrsegs = 1;
			lines = realloc( lines, nrlines * sizeof( char ** ) );
			thisline = &lines[nrlines-1];
			*thisline = malloc( nrsegs * sizeof( char* ) );
		}
		else if( c == ',' )
		{
			filedata[i] = 0;
			nrsegs++;
			*thisline = realloc( *thisline, nrsegs * sizeof( char* ) );
		}
		if( c == '\n' || c == ',' )
		{
			(*thisline)[nrsegs-1] = (filedata+i+1);
		}
	}
	
	return nrlines;
}
