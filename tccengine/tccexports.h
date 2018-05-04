/*
	Copyright (c) 2014-2018 <>< Charles Lohr
	All rights reserved.

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

#ifndef _TCCEXPORTS_H
#define _TCCEXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef INCLUDEME
INCLUDEME
#endif

//This file contains all of the things available to the scripts
void PrintHello();

//Except this function, which is the callback for populating the exports.
#ifndef TCC
#include "tccengine.h"
void PopulateTCCE(TCCEngine * tce );
#else
//various util functions
float sinf( float f );
float powf( float b, float e );
float cosf( float f );
float sqrtf( float f );
float atan2f( float a, float b );

int abs( int i );
double sin( double f );
double pow( double b, double e );
double cos( double f );
double tan( double f );
double sqrt( double f );
double atan2( double a, double b );
double atof( const char * s );

const char * strstr( const char * needle, const char * haystack );

int printf(const char *format, ...);
int sprintf(char * buf, const char *format, ...);
int sscanf(const char *format, ...);
int strcmp( const char * a, const char * b );
int strlen( const char * stn );
int memcpy( void * o, const void * i, int cp );

int rounddoubletoint( double d );
int roundfloattoint( float f );

#endif

extern struct TCCEngine * tccengine;

//extern void * cid; //Course ID
#ifdef TCC
#ifdef WIN32
#define cid ((void*)cidval)
#else
extern void * cid;
#endif
#endif

extern struct TCCEngine * gtce;

//////
////// Add additional header files or definitions you want to be available to TinyCC here.
//////


#endif

#ifdef __cplusplus
};
#endif


