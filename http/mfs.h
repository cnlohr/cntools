//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

//Not to be confused with MFS for the AVR.

#ifndef _MFS_H
#define _MFS_H

#include <stdint.h>

#ifndef USE_RAM_MFS
#include <stdio.h>
#endif

#define MFS_SECTOR	256
#define MFS_FILENAMELEN 32-8

//Format:
//  [FILE NAME (24)] [Start (4)] [Len (4)]
//  NOTE: Filename must be null-terminated within the 24.
struct MFSFileEntry
{
	char name[MFS_FILENAMELEN];
	uint32_t start;  //From beginning of mfs thing.
	uint32_t len;
};


struct MFSFileInfo
{
	uint32_t filelen;
#ifdef USE_RAM_MFS
	uint32_t offset;
#else
	FILE * file;
#endif
};



//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t MFSOpenFile( const char * fname, struct MFSFileInfo * mfi );
int32_t MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi ); //returns # of bytes left in file.
void MFSClose( struct MFSFileInfo * mfi );



#endif


