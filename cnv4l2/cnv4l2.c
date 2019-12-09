//Copyright <>< 2019 Charles Lohr, distributed without restriction.
//based on https://gist.github.com/maxlapshin/1253534, which may be distributed without restriction.

#include <cnv4l2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>


#define CLEAR(x) memset(&(x), 0, sizeof(x))

int cnv4l2_enumerate( void * opaque, cnv4l2enumcb cb )
{
    struct v4l2_capability video_cap;
	int camcount = 0;
	int i;
	for( i = 0; i < 64; i++ )
	{
		char cts[64];
		sprintf( cts, "/dev/video%d", i );
		int fd = open( cts, O_RDONLY );
		if( fd < 0 ) continue;
	    if( ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1) continue;
		if (!(video_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) continue;

		cb( opaque, cts, video_cap.card, video_cap.bus_info );
		close( fd );
	}
	return camcount;
}

cnv4l2 * cnv4l2_open( const char * path, int w, int h, cnv4l2format fmt, cnv4l2mode mode, cnv4l2callback cb )
{
	cnv4l2 * v = 0;
	//Step 1: Validate inputs a little.
	if( fmt < CNV4L2_RAW || fmt >= CNV4L2_FMT_MAX )
	{
		fprintf( stderr, "Error: cnv4l2_open(...) fmt wrong\n" );
		goto failure;
	}

	//Step 2: Make sure we have a special device.
	struct stat st;
	if ( -1 == stat( path, &st ) )
	{
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", path, errno, strerror( errno ) );
		goto failure;
	}
	if( !S_ISCHR( st.st_mode ) )
	{
		fprintf(stderr, "Device type error %s is no device\n", path);
		goto failure;
	}
	v = malloc( sizeof( cnv4l2 ) );
	v->fd = 0;
	v->mode = mode;
	v->fmt = fmt;
	v->w = w;
	v->h = h;
	v->buffersize = 0;
	v->nbuffers = 0;
	v->buffers = 0;
	v->callback = cb;
	int fd = v->fd = open( path, O_RDWR | O_NONBLOCK );
	if( !v->fd )
	{
		fprintf( stderr, "Cannot open device %s (%s)\n", path, strerror( errno ) );
		goto failure;
	}

	//Device opened.
	struct v4l2_capability cap;

	if ( -1 == ioctl( fd, VIDIOC_QUERYCAP, &cap ) ) {
		if (EINVAL == errno) {
			fprintf( stderr, "%s is no V4L2 device\n", path );
			goto failure;
		} else {
			fprintf( stderr, "%s V4L2 device querycap failed\n", path );
			goto failure;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf( stderr, "%s is no video capture device\n", path );
		goto failure;
	}


    switch ( mode ) {
		case CNV4L2_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf( stderr, "%s does not support read i/o\n", path);
				goto failure;
			}
			break;
		case CNV4L2_MMAP:
		case CNV4L2_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf( stderr, "%s does not support streaming i/o\n", path );
				goto failure;
			}
			break;
    }

	//Reset cropping, but don't worry about return failures.  Not all video hardware supports this.
	{
		struct v4l2_cropcap cropcap;
		struct v4l2_crop crop;
		CLEAR(cropcap);
		cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl(fd, VIDIOC_CROPCAP, &cropcap);
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;
		ioctl(fd, VIDIOC_S_CROP, &crop);
	}

	{
		unsigned int min = 0;
		struct v4l2_format fmtt;
		CLEAR(fmtt);
		fmtt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if( 1 /*Force format*/ )
		{
			fmtt.fmt.pix.width       = w;
			fmtt.fmt.pix.height      = h;
			fmtt.fmt.pix.pixelformat = (const uint32_t[]){ V4L2_PIX_FMT_SRGGB8, V4L2_PIX_FMT_YUYV,  V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_H264 }[fmt]; //replace
			fmtt.fmt.pix.field       = V4L2_FIELD_ANY;

			if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmtt))
			{
				fprintf( stderr, "Error: %s: Cannot set format for H264 video stream\n", path );
				/* Note VIDIOC_S_FMT may change width and height. */
				goto failure;
			}
		}
		else
		{
			/* Preserve original settings as set by v4l2-ctl for example */
			if (-1 == ioctl(fd, VIDIOC_G_FMT, &fmtt))
			{
				fprintf( stderr, "Error: VIDIOC_G_FMT in %s\n", path );
				goto failure;
			}
		}
		/* Buggy driver paranoia. */
		min = fmtt.fmt.pix.width * 2;
		if (fmtt.fmt.pix.bytesperline < min)
			fmtt.fmt.pix.bytesperline = min;
		min = fmtt.fmt.pix.bytesperline * fmtt.fmt.pix.height;
		if (fmtt.fmt.pix.sizeimage < min)
			fmtt.fmt.pix.sizeimage = min;

		v->buffersize = fmtt.fmt.pix.sizeimage;
	}


	switch (mode) {
	case CNV4L2_READ:
		v->nbuffers = 1;
		v->buffers = malloc( sizeof(uint8_t*)*v->nbuffers );
		v->buffers[0] = malloc(v->buffersize);
		break;
	case CNV4L2_MMAP:
	{
        struct v4l2_requestbuffers req;
        CLEAR(req);
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

		if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
			if (EINVAL == errno) {
				fprintf( stderr, "%s does not support memory mapping\n", path );
			} else {
				fprintf( stderr, "%s VIDIOC_REQBUFS errors\n", path );
			}
			goto failure;
		}

		int count = req.count;

		if ( count < 2 ) {
			fprintf( stderr, "Insufficient buffer memory on %s\n", path );
			goto failure;
		}

		v->nbuffers = count;
		v->buffers = malloc( sizeof(uint8_t*)*count );

		int idx;
		for ( idx = 0; idx < count; ++idx ) {
			struct v4l2_buffer buf;
			CLEAR(buf);
			buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = idx;

			if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
			{
				fprintf( stderr, "VIDIOC_QUERYBUF - %s fault\n", path );
			}

			v->buffersize = buf.length;

			v->buffers[idx] = mmap(NULL /* start anywhere */,
				buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */,
				fd, buf.m.offset );

			if (MAP_FAILED == v->buffers[idx] )
			{
				fprintf( stderr, "Error: %s can't mmap\n", path );
				goto failure;
			}
		}
		break;
	}
	case CNV4L2_USERPTR:
	{
		struct v4l2_requestbuffers req;
		CLEAR(req);
		req.count  = 4;
		req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_USERPTR;

		if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
			if (EINVAL == errno) {
				fprintf( stderr, "%s does not support user pointer i/o\n", path );
				goto failure;
			} else {
				fprintf( stderr, "%s does not support user pointer i/o - VIDIOC_REQBUFS\n", path );
				goto failure;
			}
		}
		int count = v->nbuffers = req.count;
		v->buffers = malloc( sizeof(uint8_t*)*count );

		int n_buffers;
		for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
			v->buffers[n_buffers] = malloc(v->buffersize);
		}

		break;
	}
	}

	return v;

failure:
	if( v->fd ) close( v->fd );
	{
		int i;
		for( i = 0; i < v->nbuffers; i++ )
		{
			free( v->buffers[i] );
		}
		free( v->buffers );
	}
	if( v ) free( v );
	return 0;

}

int cnv4l2_start_capture( cnv4l2 * v )
{
	//Actually start capture.
	{
		unsigned int i;
		enum v4l2_buf_type type;
		switch (v->mode)
		{
		case CNV4L2_READ:
			/* Nothing to do. */
			break;

		case CNV4L2_MMAP:
			for (i = 0; i < v->nbuffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;

				if (-1 == ioctl(v->fd, VIDIOC_QBUF, &buf))
				{
					fprintf( stderr, "Startin capture fail VIDIOC_QBUF (%d)", v->fd );
					goto failure;
				}
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == ioctl(v->fd, VIDIOC_STREAMON, &type))
			{
				fprintf( stderr, "Startin capture fail VIDIOC_STREAMON (%d)", v->fd );
				goto failure;
			}
			break;

		case CNV4L2_USERPTR:
			for (i = 0; i < v->nbuffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;
				buf.index = i;
				buf.m.userptr = (unsigned long)v->buffers[i];
				buf.length = v->buffersize;

				if (-1 == ioctl(v->fd, VIDIOC_QBUF, &buf))
				{
					fprintf( stderr, "Startin capture fail VIDIOC_QBUF (%d)", v->fd );
					goto failure;
				}
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == ioctl(v->fd, VIDIOC_STREAMON, &type))
			{
				fprintf( stderr, "Startin capture fail VIDIOC_STREAMON (%d)", v->fd );
				goto failure;
			}
			break;
		}
	}
	return 0;
failure:
	close( v->fd );
	v->fd = 0;
	return -1;
}

int cnv4l2_read( cnv4l2 * v, uint64_t timeoutusec )
{
	if( !v || !v->fd ) return -1;
	{
		//Use select to see if there is a pending frame and/or wait on it.
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(v->fd, &fds);

		/* Timeout. */
		tv.tv_sec = timeoutusec/1000000;
		tv.tv_usec = timeoutusec%1000000;

		r = select(v->fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				goto failure;
			}

			if (0 == r) {
				return 0;
		}
	}



	//First select.


	struct v4l2_buffer buf;
	unsigned int i;
	int r;
	switch ( v->mode ) {
	case CNV4L2_READ:
		r = read(v->fd, v->buffers[0], v->buffersize);
		if ( -1 == r ) {
			switch (errno) {
				case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				goto failure;
			}
		}
		v->callback( v, v->buffers[0], r );
		break;
	case CNV4L2_MMAP:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == ioctl(v->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				fprintf( stderr, "v4l2 VIDIOC_DQBUF fail\n" );
				return 0;
			}
		}
		if( buf.index >= v->nbuffers )
		{
			fprintf( stderr, "Error: bad index returned on mmap for v4l2\n" );
			goto failure;
		}

		v->callback( v, v->buffers[buf.index], buf.bytesused );

		if (-1 == ioctl(v->fd, VIDIOC_QBUF, &buf))
		{
			fprintf( stderr, "v4l2 VIDIOC_QBUF fail" );
			goto failure;
		}
		break;
	case CNV4L2_USERPTR:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == ioctl(v->fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				fprintf( stderr, "v4l2 VIDIOC_DQBUF fail\n" );
				return 0;
			}
		}

		v->callback( v, (uint8_t*)buf.m.userptr, buf.length );

		if (-1 == ioctl(v->fd, VIDIOC_QBUF, &buf))
		{
			fprintf( stderr, "v4l2 VIDIOC_QBUF fail" );
			goto failure;
		}
		break;
	}

	return 1;
failure:
	close( v->fd );
	v->fd = 0;
	return -1;

}

int cnv4l2_set_framerate( cnv4l2 * v, int num, int denom )
{
    struct v4l2_streamparm parm;
	CLEAR( parm );
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.capability=V4L2_CAP_TIMEPERFRAME;
    parm.parm.capture.timeperframe.numerator = num;
    parm.parm.capture.timeperframe.denominator = denom;

    int ret = ioctl(v->fd, VIDIOC_S_PARM, &parm);
	printf( "RATER: %d\n", ret );
    if( ret < 0 ) return ret;
	ret = ioctl(v->fd, VIDIOC_G_PARM, &parm);
	if( ret < 0 ) return 0;
	printf( "Rate: %d/%d\n", parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator );
    return 1;
}

void cnv4l2_close( cnv4l2 * v )
{
	if( v->fd ) close( v->fd );
	{
		int i;
		for( i = 0; i < v->nbuffers; i++ )
		{
			if( v->mode == CNV4L2_MMAP )
				munmap( v->buffers[i], v->buffersize );
			else
				free( v->buffers[i] );
		}
		free( v->buffers );
	}
	if( v ) free( v );
}


