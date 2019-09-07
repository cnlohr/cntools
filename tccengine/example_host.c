#include <stdio.h>
#include <os_generic.h>
#include "tccengine.h"
#include "tccexports.h"
#include "tcccrash.h"

#include <execinfo.h>

#include "tcc/libtcc.h"

void PopFn( TCCEngine * tce )
{
	PopulateTCCE( tce );
}
TCCEngine * e;

int ecaller()
{
	double start = OGGetAbsoluteTime();
	int k = tcccrash_checkpoint();
	if( !k )
		if( e->update ) e->update( e->cid );
	printf( "Total time: %f\n", OGGetAbsoluteTime() - start );
	return 0;
}

void PostFn( TCCEngine * tce )
{
	printf( "POST %p %p\n", tce->state, tce->backuptcc );
	if( tce->backuptcc ) tcccrash_deltag( (intptr_t)tce->backuptcc );
	tcccrash_symtcc( tce->filename, tce->state );
}

int main()
{
	unsigned char tcceb[8192];

	tcccrash_install();

	e = TCCECreate( "example_script.c", 0, 0, PopFn, PostFn, tcceb );
	printf( "Created\n" );
	int i;
	printf( "Starting %p %p\n", main, ecaller );
	//gettimeofday


	while(1)
	{
		TCCECheck( e, 0 );
		ecaller();
		//uint32_t * localk = tcccrash_symaddr( e->state, "k" );
		//printf( "%p\n", localk );
		//printf( "%d\n", *localk );


		OGUSleep( 100000 );
	}
}

