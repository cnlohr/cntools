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

#ifndef _TCCENGINE_H
#define _TCCENGINE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBTCC_H
typedef struct TCCState TCCState;
#endif

#define MAX_ATTACHED_FILES 20

typedef struct TCCEngine TCCEngine;

typedef void (*PopulateTCCEFunction)(TCCEngine * tce );
typedef int (*TCELinkage)( void * v );

struct TCCEngine
{
	//Dupped
	const char * filename;
	double readtime;

	const char * attachedp[MAX_ATTACHED_FILES];
	double attachedtp[MAX_ATTACHED_FILES];

	TCCState * state;
	PopulateTCCEFunction popfn;
	void * image;

	TCELinkage init;  //called the very first run
	TCELinkage start; //called any time the script is loaded/reloaded
	TCELinkage stop;  //called any time the script is unloaded (For reloading)
	TCELinkage update;//called every frame.

	void * cid;
};


//will not dup the "attachedfiles" you need to dup it or not delete it.
TCCEngine * TCCECreate( const char * tccfile, const char ** attachedfiles, int attachecount, PopulateTCCEFunction  popfn, void * cid );

int TCCECheck( TCCEngine * tce, int first ); //returns 1 if re-compiling good. 0 if nothing was done.
void TCCSetDefine( TCCEngine * tce, const char * symname, const char * sym );
void TCCEDestroy( TCCEngine * tce );
void * TCCEGetSym( TCCEngine * tce, const char * symname );
void TCCESetSym( TCCEngine * tce, const char * symname, void * sym );

#ifdef __cplusplus
};
#endif


#endif


