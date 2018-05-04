#include "tccexports.h"
#include "../os_generic.h"

struct ScriptStructure
{
	int compiles;
};

int init( struct ScriptStructure * cid )
{
	printf( "Init\n" );
}

int start( struct ScriptStructure * cid )
{
	printf( "Start\n" );
	cid->compiles++;
}

int stop( struct ScriptStructure * cid )
{
	printf( "Stop\n" );
}

int update( struct ScriptStructure * cid )
{
	printf( "Update: %d %f\n", cid->compiles, OGGetAbsoluteTime() );
}



