/*
	Copyright (c) 2014-2019 <>< Charles Lohr
	All rights reserved.  You may license this file under the MIT/x11, NewBSD,
		or CC0 licenses, your choice.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.
		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.
		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	This file may also be licensed under the MIT/x11 license if you wish.
*/

#include <libtcc.h>
#include "tccengine.h"
#include "os_generic.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//TinyCC uses a lot of global operations, we cannot multithread its operation.
static og_mutex_t TCCMutex;


TCCEngine * TCCECreate( const char * tccfile, const char ** attachedfiles, int attachedcount, PopulateTCCEFunction  popfn, PopulateTCCEFunction  postfn, void * cid )
{
	if( !TCCMutex ) TCCMutex = OGCreateMutex();
	OGLockMutex( TCCMutex );

	int i;
	TCCEngine * ret = malloc( sizeof( TCCEngine ) );
	memset( ret, 0, sizeof( TCCEngine ) );
	ret->filename = strdup( tccfile );
	ret->popfn = popfn;
	ret->postfn = postfn;
	ret->cid = cid;


	if( attachedcount > MAX_ATTACHED_FILES )
	{
		free( ret );
		fprintf( stderr, "ERROR: Too many attached files.  Recompile with higher MAX_ATTACHED_FILES\n" );
		OGUnlockMutex( TCCMutex );
		return 0;
	}
	for( i = 0; i < attachedcount; i++ )
	{
		ret->attachedp[i] = attachedfiles[i];
	}

	for( ; i < MAX_ATTACHED_FILES; i++ )
	{
		ret->attachedtp[i] = 0;
		ret->attachedp[i] = 0;
	}

	OGUnlockMutex( TCCMutex );

	TCCECheck( ret, 1 );
	return ret;
}

int TCCECheck( TCCEngine * tce, int first )
{
	OGLockMutex( TCCMutex );

	int r, i;
/*
	char * program;
	int ifl;
	FILE * f = fopen( tce->filename, "rb" );
	if( !f || ferror( f ) )
	{
		if( f ) fclose( f );
		fprintf( stderr, "Error: Cannot open TCC Tool on file: \"%s\"\n", tce->filename );
		return;
	}
	fseek( f, 0, SEEK_END );
	ifl = ftell( f );
	fseek( f, 0, SEEK_SET );
	program = malloc( ifl + 1 );
	int r = fread( program, ifl, 1, f );
	program[ifl] = 0;
	fclose( f );
	if( r < 0 )
	{
		fprintf( stderr, "Error: File I/O Error on file: \"%s\"\n", tce->filename );
		free( program );
		return;
	}*/

	int earliercheck = 1;
	for( i = 0; i < MAX_ATTACHED_FILES; i++ )
	{
		if( !tce->attachedp[i] ) break;
		double time = OGGetFileTime( tce->attachedp[i] );
		if( OGGetFileTime( tce->attachedp[i] ) != tce->attachedtp[i] )
		{
			earliercheck = 0;
			tce->attachedtp[i] = time;
		}
	}


	double readtime = OGGetFileTime( tce->filename );
	if( tce->readtime == readtime && earliercheck )
	{
		OGUnlockMutex( TCCMutex );
		return 0;
	}

	OGUSleep( 1000 ); //On Windows, sometimes files get corrupt if you try reading them too soon.

	tce->readtime = readtime;
	tce->backuptcc = tce->state;
	
	tce->state = tcc_new();
	tcc_set_output_type(tce->state, TCC_OUTPUT_MEMORY);
	tce->popfn( tce ); //Populate everything.

	char cts[1024];
	sprintf( cts,"0x%p", tce->cid );
	tcc_define_symbol( tce->state, "cidval", cts );

	tcc_add_library( tce->state, "m" );
	tcc_add_include_path( tce->state, ".." );
	tcc_add_include_path( tce->state, "../cntools/tccengine/tcc/include_tcc" );
	tcc_add_include_path( tce->state, "cntools/tccengine/tcc/include_tcc" );
	tcc_add_include_path( tce->state, "../../cntools/tccengine/tcc/include_tcc" );
	tcc_add_include_path( tce->state, "tcc/include_tcc/include" );
	tcc_add_include_path( tce->state, "." );
	tcc_define_symbol( tce->state, "TCC", 0 );
	tcc_set_options( tce->state, "-nostdlib -rdynamic" );

#ifdef WIN32
	tcc_define_symbol( tce->state, "WIN32", "1" );
#endif

	r = tcc_add_file( tce->state, tce->filename );
	if( r )
	{
		fprintf( stderr, "TCC Comple Status: %d\n", r );
		OGUnlockMutex( TCCMutex );
		goto end_err;
	}

#if 0
#if __WORDSIZE == 64
 	tcc_add_file(tce->state, "tcc/libtcc1-x86_64.a");
#else
// 	tcc_add_file(tce->state, "/usr/local/lib/tcc/libtcc1.a", 1);
//#error No TCC Defined for this architecture.
#endif
#endif
	r = tcc_relocate(tce->state, NULL);

	if (r == -1)
	{
		fprintf( stderr, "TCC Reallocation error." );
		goto end_err;
	}

	//Issue stop
	if( tce->stop )
	{
		printf( "Stopping...\n" );
		OGUnlockMutex( TCCMutex );
		tce->stop( tce->cid );
		OGLockMutex( TCCMutex );
		printf( "Stop done.\n" );
	}
	void * backupimage = tce->image;
	tce->image = malloc(r);
	tcc_relocate( tce->state, tce->image );
	if( tce->postfn ) tce->postfn( tce );
	if( tce->backuptcc )
		tcc_delete( tce->backuptcc );

	if( backupimage ) free( backupimage );

	tce->stop = (TCELinkage)tcc_get_symbol(tce->state, "stop" );
	tce->init = (TCELinkage)tcc_get_symbol(tce->state, "init" );
	tce->start = (TCELinkage)tcc_get_symbol(tce->state, "start" );
#ifndef NOTCCUPDATE
	tce->update = (TCELinkage)tcc_get_symbol(tce->state, "update" );
#endif

	if( first && tce->init) { tce->init( tce->cid ); }

	OGUnlockMutex( TCCMutex );
	if( !tce->start )
	{
		fprintf( stderr, "Warning: Cannot find start function in %s.\n", tce->filename );
	}
	else
	{
		tce->start( tce->cid );
	}

	return 1;
end_err:
	tcc_delete( tce->state );
	tce->state = tce->backuptcc;
	return -1;
}

void TCCEDestroy( TCCEngine * tce )
{
	OGLockMutex( TCCMutex );
	tcc_delete( tce->state );
	free( tce->image );
	free( tce );
	OGUnlockMutex( TCCMutex );
}

void * TCCEGetSym( TCCEngine * tce, const char * symname )
{
	void * ret;
	OGLockMutex( TCCMutex );
	ret = tcc_get_symbol(tce->state, symname );
	OGUnlockMutex( TCCMutex );
	return ret;
}

void TCCSetDefine( TCCEngine * tce, const char * symname, const char * sym )
{
	OGLockMutex( TCCMutex );
	tcc_define_symbol( tce->state, symname, sym );
	OGUnlockMutex( TCCMutex );
}

void TCCESetSym( TCCEngine * tce, const char * symname, void * sym )
{
	OGLockMutex( TCCMutex );
	tcc_add_symbol( tce->state, symname, sym );
	OGUnlockMutex( TCCMutex );
}




