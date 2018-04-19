//This file is included in the parent application.

#ifndef _TCCENGINE_H
#define _TCCENGINE_H

typedef struct
{
	char * filename;
	double filetime;
	TCCState * state;

	int (*Start)( void * v );
	int (*Stop)( void * v );
} TCCEngine;

TCCEngine * TCCEngineCreate( const char * filename );
//You can do other things to the t->state here, like tcc_set_error_func, etc.
int TCCEngineCheckCompile( TCCEngine * t );
void TCCEngineDestroy( TCCEngine * t );


#endif

