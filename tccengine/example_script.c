#include "tccexports.h"
#include "../os_generic.h"

static int k;

struct ScriptStructure
{
	int compiles;
};


int inner2()
{
	if( k % 10 == 9 )
	{
		char * v = 0;
		*v = 5;
	}
}



int init( struct ScriptStructure * cid )
{
	printf( "Init\n" );
}




int start( struct ScriptStructure * cid )
{
	printf( "Start\n" );
	k = 0;
	cid->compiles++;
}


int inner()
{
	k++;
	inner2();
}


int stop( struct ScriptStructure * cid )
{
	printf( "Stop\n" );
}


int update( struct ScriptStructure * cid )
{
	inner();
	printf( "Update: %d %f\n", cid->compiles, OGGetAbsoluteTime() );
}



