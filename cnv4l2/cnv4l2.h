//Copyright <>< 2019 Charles Lohr, distributed without restriction.
//based on https://gist.github.com/maxlapshin/1253534, which may be distributed without restriction.

#ifndef _CNV4L2_H
#define _CNV4L2_H

#include <stdint.h>

//Specifically uses mmap method.
//Currently only supports yuyv, mjpeg

typedef enum
{
	CNV4L2_RAW = 0,
	CNV4L2_YUYV,
	CNV4L2_MJPEG,
	CNV4L2_H264,	//Not fully supported.
	CNV4L2_FMT_MAX,
} cnv4l2format;

typedef enum
{
	CNV4L2_READ,
	CNV4L2_MMAP,
	CNV4L2_USERPTR,
} cnv4l2mode;

struct cnv4l2_t;

typedef void (*cnv4l2callback)( struct cnv4l2_t * match, uint8_t * payload, int payloadlen );
typedef void (*cnv4l2enumcb)( void * opaque, const char * dev, const char * name, const char * bus );

struct cnv4l2_t
{
	int fd;
	cnv4l2mode mode;
	cnv4l2format fmt;
	int w, h;
	void * opaque;
	cnv4l2callback callback;
	uint32_t buffersize;
	uint32_t nbuffers;
	uint8_t ** buffers;
};

typedef struct cnv4l2_t cnv4l2;

int cnv4l2_enumerate( void * opaque, cnv4l2enumcb cb );

cnv4l2 * cnv4l2_open( const char * path, int w, int h, cnv4l2format fmt, cnv4l2mode mode, cnv4l2callback cb );
int cnv4l2_set_framerate( cnv4l2 * v, int num, int denom );
int cnv4l2_start_capture( cnv4l2 * v );
int cnv4l2_read( cnv4l2 * v, uint64_t timeoutusec ); //'callback' called in this context.
void cnv4l2_close( cnv4l2 * v );

#endif

