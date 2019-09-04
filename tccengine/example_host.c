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
	if( e->update ) e->update( e->cid );
}

int main()
{
	unsigned char tcceb[8192];

	tcccrash_install();


	e = TCCECreate( "example_script.c", 0, 0, PopFn, tcceb );
	printf( "Created\n" );
	int i;

	char ** n;
	void ** v;
	int ** f;
	int r = tcc_get_all_symbols( e->state, &n, &v, &f );
	int k;
	printf( "RET: %d\n", r );
	for( k = 0; k < r; k++ )
	{
		printf( "R: %p %d %s\n", v[k], f[k], n[k] );
		tcccrash_syminfo * symadd = malloc( sizeof( tcccrash_syminfo ) );
		symadd->tag = 1;
		symadd->name = strdup( n[k] );
		symadd->path = ".";
		symadd->size = 1024; //XXX TODO
		symadd->address = v[k];
		tcccrash_symset( e->state, symadd );
		printf( "OK\n" );
	}

	printf( "Starting\n" );
	//gettimeofday


	while(1)
	{
		TCCECheck( e, 0 );
		printf( "CPIN\n" );
		int k = tcccrash_checkpoint();
		printf( "K: %d\n", k );
		if( k == 11 ) i = 0;
		printf( "DOLOOP [%p] [%p]\n", &main, &ecaller );
		ecaller();
		printf( "DONE %p\n", &main );
		printf( "DONE2\n" );
		OGUSleep( 100000 );
	}
}

