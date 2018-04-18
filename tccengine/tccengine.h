//This file is included in the parent application.

#ifndef _TCCENGINE_H
#define _TCCENGINE_H

typedef struct
{
	char * filename;
	double filetime;
	

	int (*Start)( void * v );
	int (*Stop)( void * v );
} TCCEngine;

TCCEngine * TCCEngineCreate( const char * filename );
int TCCEngineCheck();


#endif

