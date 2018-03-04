//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "mfs.h"

#ifdef USE_RAM_MFS
#include "mfs_data.h"
#endif

#include <string.h>

#ifdef USE_RAM_MFS

//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t MFSOpenFile( const char * fname, struct MFSFileInfo * mfi )
{
	struct MFSFileEntry e;
	uint8_t * ptr = mfs_data;
	while(1)
	{
		//spi_flash_read( ptr, (uint32*)&e, sizeof( e ) );		
		memcpy( &e, ptr, sizeof( e ) );
		ptr += sizeof(e);
		if( e.name[0] == 0xff || strlen( e.name ) == 0 ) break;

		if( strcmp( e.name, fname ) == 0 )
		{
			mfi->offset = e.start;
			mfi->filelen = e.len;
			return 0;
		}
	}
	return -1;
}

int32_t MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi )
{
	 //returns # of bytes left tin file.
	if( !mfi->filelen )
	{
		return 0;
	}

	int toread = mfi->filelen;
	if( toread > MFS_SECTOR ) toread = MFS_SECTOR;
	memcpy( data, &mfs_data[mfi->offset], toread );
	mfi->offset += toread;
	mfi->filelen -= toread;
	return mfi->filelen;
}

void MFSClose( struct MFSFileInfo * mfi )
{
}

#else


//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t MFSOpenFile( const char * fname, struct MFSFileInfo * mfi )
{
	char targfile[1024];

	if( strlen( fname ) == 0 || fname[strlen(fname)-1] == '/' )
	{
		snprintf( targfile, sizeof( targfile ) - 1, "page/%s/index.html", fname );
	}
	else
	{
		snprintf( targfile, sizeof( targfile ) - 1, "page/%s", fname );
	}

	printf( ":%s:\n", targfile );

	FILE * f = mfi->file = fopen( targfile, "rb" );
	if( f <= 0 ) return -1;
	printf( "F: %p\n", f );
	fseek( f, 0, SEEK_END );
	mfi->filelen = ftell( f );
	fseek( f, 0, SEEK_SET );
	return 0;
}

int32_t MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi )
{
	if( !mfi->filelen )
	{
		return 0;
	}

	int toread = fread( data, 1, MFS_SECTOR, mfi->file );
	mfi->filelen -= toread;
	return mfi->filelen;
}

void MFSClose( struct MFSFileInfo * mfi )
{
	if( mfi->file ) fclose( mfi->file );
}


#endif


