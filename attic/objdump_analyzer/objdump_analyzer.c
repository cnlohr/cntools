#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int verbose;
int do_help;
const char * filename = "-";
const char * outname = "index.html";

char ** SplitStrings( const char * line, char * split, char * white, int merge_fields );

int main( int argc, char ** argv )
{
	int opt;

	while ((opt = getopt(argc, argv, "f:hvo:")) != -1) {
		switch (opt) {
			case 'h': do_help = 1; break;
			case 'v': verbose = 1; break;
			case 'f': filename = optarg; break;
			case 'o': outname = optarg; break;
			default: do_help = 1; break;
		}
	}
	if( do_help )
	{
		fprintf( stderr, "objdump_analyzer: Usage:\n\
	-f [input filename (output from objdump -t), - for stdin (default)]\n\
	-h Show help\n\
	-v Verbose output\n\
	-o [output HTML file, -o- will output to stdout]\n" );
		return -1;
	}

	FILE * fin = 0;
	FILE * fout = 0;

	if( strcmp( filename, "-" ) == 0 )	fin = stdin;
	else fin = fopen( filename, "r" );

	if( !fin || feof( fin ) || ferror( fin ) )
	{
		fprintf( stderr, "Fault on input file %s\n", filename );
		return -2;
	}

	char * indata = malloc( 1 );
	int len = 0;
	int c;
	while( ( c = fgetc( fin ) ) != EOF ) { indata[len++] = c; indata = realloc( indata, len + 1); }
	indata[len] = 0;
	fclose( fin );

	puts(indata );
	char ** lines = SplitStrings( indata, "\n", "", 1 );
	char ** oline = lines;
//0000000000000238 l    d  .interp	0000000000000000              .interp
//0000000000000254 l    d  .note.ABI-tag	0000000000000000              .note.ABI-tag
	char * l;

	while( l = *(lines++) )
	{
		char ** segs = SplitStrings( l, " ", " ", 1 );
		int m;
		printf( "L: %s\n", l );
		for( m = 0; m < 5; m++ )
		{
			printf( "%s/", segs[m] );
		}
		printf( "\n" );

	}
	free( oline );

}









char ** SplitStrings( const char * line, char * split, char * white, int merge_fields )
{
	if( !line || strlen( line ) == 0 )
	{
		char ** ret = malloc( sizeof( char * )  );
		*ret = 0;
		return ret;
	}

	int elements = 1;
	char ** ret = malloc( elements * sizeof( char * )  );
	int * lengths = malloc( elements * sizeof( int ) ); 
	int i = 0;
	char c;
	int did_hit_not_white = 0;
	int thislength = 0;
	int thislengthconfirm = 0;
	int needed_bytes = 1;
	const char * lstart = line;
	do
	{
		int is_split = 0;
		int is_white = 0;
		char k;
		c = *(line);
		for( i = 0; (k = split[i]); i++ )
			if( c == k ) is_split = 1;
		for( i = 0; (k = white[i]); i++ )
			if( c == k ) is_white = 1;

		if( c == 0 || ( ( is_split ) && ( !merge_fields || did_hit_not_white ) ) )
		{
			//Mark off new point.
			lengths[elements-1] = thislengthconfirm + 1; //XXX BUGGY ... Or is bad it?  I can't tell what's wrong.  the "buggy" note was from a previous coding session.
			ret[elements-1] = (char*)lstart + 0; //XXX BUGGY //I promise I won't change the value.
			needed_bytes += thislengthconfirm + 1;
			elements++;
			ret = realloc( ret, elements * sizeof( char * )  );
			lengths = realloc( lengths, elements * sizeof( int ) );
			lstart = line;
			thislength = 0;
			thislengthconfirm = 0;
			did_hit_not_white = 0;
			line++;
			continue;
		}

		if( !is_white && ( !(merge_fields && is_split) ) )
		{
			if( !did_hit_not_white )
			{
				lstart = line;
				thislength = 0;
				did_hit_not_white = 1;
			}
			thislengthconfirm = thislength;
		}

		if( is_white )
		{
			if( did_hit_not_white ) 
				is_white = 0;
		}

		if( did_hit_not_white )
		{
			thislength++;
		}
		line++;
	} while ( c );

	//Ok, now we have lengths, ret, and elements.
	ret = realloc( ret, sizeof( char * ) * elements  + needed_bytes );
	char * retend = ((char*)ret) + (sizeof( char * ) * elements);

	for( i = 0; i < elements; i++ )
	{
		int len = lengths[i];
		memcpy( retend, ret[i], len );
		retend[len] = 0;
		ret[i] = (i == elements-1)?0:retend;
		retend += len + 1;
	}
	free( lengths );
	return ret;
}
