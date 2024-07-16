#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUFLEN (1024*1024*10)
void error(const char *msg);
void gz_uncompress(gzFile in, int out);

char buf[BUFLEN];

int main( int argc, char ** argv )
{
	gzFile zfp;
	if( argc != 3)
	{
		fprintf( stderr, "Error: Usage: synczcat [.gz file] [where to overwrite]\n" );
	}

	int outf = open( argv[2], O_WRONLY | O_SYNC | O_CREAT, 0666 );

	if( outf <= 0 )
	{
		fprintf( stderr, "Error: can't open \"%s\" for writing.\n", argv[2] );
		exit( -4 );
	}

	/* file_uncompress(*argv); */
	zfp = gzopen(argv[1], "rb");
	if (zfp == NULL) {
		fprintf(stderr, "%s: can't gzopen %s\n", argv[0], argv[1]);
		exit(-3);
	}
	gz_uncompress(zfp, outf);

	return 0;
}


void gz_uncompress(gzFile in, int out)
{
	int len;
	int err;
	int mb = 0;
	int r;

	printf( "Writing to %d\n", out );

	for (;;) {
		len = gzread(in, buf, sizeof(buf));
		if (len < 0)
			error (gzerror(in, &err));
		if (len == 0)
			break;

		r = write( out, buf, len );
		if( r != len )
		{
			fprintf( stderr, "Failed write (%d/%d).\n", r, len );
			exit( -10 );
		}
		sync();
		mb+=10;
	
		printf( "%d MB\n", mb );
	}
	if (close(out))
	{
		fprintf(stderr,"failed fclose");
		exit( -3 );
	}

	if (gzclose(in) != Z_OK)
	{
		fprintf( stderr, "failed gzclose");
		exit( -9 );
	}
}




/*
 * XXX: hacks to keep gzio.c from pulling in deflate stuff
 */

int deflateInit2_(z_streamp strm, int  level, int  method,
    int windowBits, int memLevel, int strategy,
    const char *version, int stream_size)
{

	return -1;
}

int deflate(z_streamp strm, int flush)
{

	return -1;
}

int deflateEnd(z_streamp strm)
{

	return -1;
}

int deflateParams(z_streamp strm, int level, int strategy)
{

	return Z_STREAM_ERROR;
}

