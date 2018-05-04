#include <stdio.h>
#include <os_generic.h>
#include "tccengine.h"
#include "tccexports.h"


void PopFn( TCCEngine * tce )
{
	PopulateTCCE( tce );
}

int main()
{
	unsigned char tcceb[8192];

	TCCEngine * e = TCCECreate( "example_script.c", 0, 0, PopFn, tcceb );

	while(1)
	{
		TCCECheck( e, 0 );
		if( e->update ) e->update( e->cid );
		OGUSleep( 100000 );
	}
}

