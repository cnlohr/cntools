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

#define TCC_ONLY

#include <GL/gl.h>

#include "tccengine.h"
#include "tccexports.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>


int rounddoubletoint( double d ) { return d + 0.5; }
int roundfloattoint( float f ) { return f + 0.5; }

#ifdef WIN32

float Zfabs( float f ) { return fabs( f ); }
float Zsinf( float f ) { return sinf( f ); }
float Zpowf( float f, float e ) { return powf( f ,e ); }
float Zcosf( float f ) { return cosf( f ); }
float Zsqrtf( float f ) { return sqrtf( f ); }

int Zabs( int i ) { return abs(i); }

double Zsin( double f ) {return sin(f); }
double Zpow( double b, double e ) { return pow( b, e ); }
double Zcos( double f ) { return cos( f ); }
double Ztan( double f ) { return tan( f ); }
double Zsqrt( double f ) { return sqrt( f ); }
double Zatan2( double a, double b ) { return atan2( a, b ); }

#endif

int gSimMode;

void PrintHello()
{
	printf( "Script running.\n" );
}


void PopulateTCCE(TCCEngine * tce )
{
	TCCESetSym( tce, "PrintHello", PrintHello );
	TCCESetSym( tce, "printf", printf );
	TCCESetSym( tce, "gtce", &tce );

	TCCESetSym( tce, "cid", &tce->cid );

#ifdef WIN32
	TCCESetSym( tce, "sin", Zsin );
	TCCESetSym( tce, "cos", Zcos );
	TCCESetSym( tce, "pow", Zpow );
	TCCESetSym( tce, "tan", Ztan );
	TCCESetSym( tce, "atan2", Zatan2 );
	TCCESetSym( tce, "sqrt", Zsqrt );
	TCCESetSym( tce, "abs", Zabs );

	TCCESetSym( tce, "fabs", Zfabs );
	TCCESetSym( tce, "sinf", Zsinf );
	TCCESetSym( tce, "cosf", Zcosf );
	TCCESetSym( tce, "powf", Zpowf );
	TCCESetSym( tce, "sqrtf", Zsqrtf );

	TCCESetSym( tce, "sprintf", sprintf );
	TCCESetSym( tce, "strlen", strlen );
	TCCESetSym( tce, "memset", memset );
	TCCESetSym( tce, "memcpy", memcpy );
	TCCESetSym( tce, "strstr", strstr );
	TCCESetSym( tce, "sscanf", sscanf );
	TCCESetSym( tce, "strcmp", strcmp );
	TCCESetSym( tce, "atof", atof );

	extern void __fixsfdi();	TCCESetSym( tce, "__fixsfdi", __fixsfdi );
	extern void __fixdfdi();	TCCESetSym( tce, "__fixdfdi", __fixdfdi );
#else
	TCCESetSym( tce, "sinf", sinf );
	TCCESetSym( tce, "cosf", cosf );
	TCCESetSym( tce, "powf", powf );
	TCCESetSym( tce, "sqrtf", sqrtf );
#endif

	//TCCESetSym( tce, "OGGetAbsoluteTime", OGGetAbsoluteTime );
	//Add additional definitions here.
	
/*
	int i;
	char includeme[65536];
	char * includeplace = &includeme[0];
	memset( includeme, 0, sizeof( includeme ) );
	//Add stuff that you want to include in the header file here.
	for( i = 0; i < NR_MSG; i++ )
	{
		char ct[100], ct2[100];
		const char * name = VarNames[i];
		if( !name ) continue;

		includeplace += sprintf( includeplace, "extern float %s;\n", name+1 );
		TCCESetSym( tce, name+1, &SetVals[i] );

		includeplace += sprintf( includeplace, "extern float _%s;\n", name+1 );
		sprintf( ct, "_%s", name+1 );
		TCCESetSym( tce, ct, &ActVals[i] );

//		includeplace += sprintf( includeplace, "#define idof%s %d\n", name+1, i );
		sprintf( ct, "idof%s", name+ 1 );
		sprintf( ct2, "%d", i );
		TCCSetDefine( tce, ct, ct2 );
	}
	TCCSetDefine( tce, "INCLUDEME", includeme );
	*/
	TCCSetDefine( tce, "INCLUDEME", "" );
}




