#include "cnv4l2.h"
#include <os_generic.h>
#include <stdio.h>
#include <unistd.h>

double start = 0;

void mycallback( struct cnv4l2_t * match, uint8_t * payload, int payloadlen )
{
	double now = OGGetAbsoluteTime();
	printf( "%f\n", now-start );
	start = now;
}

void myenumcb( void * opaque, const char * dev, const char * name, const char * bus )
{
	printf( "%s/%s/%s\n", dev, name, bus );
}


int main()
{
	cnv4l2_enumerate( 0, myenumcb );

	cnv4l2 * v = cnv4l2_open( "/dev/video2", 1920, 1080, CNV4L2_YUYV, CNV4L2_MMAP, mycallback );
	int r = cnv4l2_set_framerate( v, 1, 60 );
	printf( "R: %d\n", r );
	r = cnv4l2_start_capture( v );
	printf( "R: %d\n", r );
	start = OGGetAbsoluteTime();
	while( 1 )
	{
		int r = cnv4l2_read( v, 1000000 );
		if( r < 0 ) break;
	}
	cnv4l2_close( v );
}


