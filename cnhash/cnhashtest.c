#include "cnhash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main()
{
	int i;
	cnhashtable * h = CNHashGenerate( 1, 0, CNHASH_STRINGS );
	CNHashInsert( h, strdup("hello"), strdup("hello") );

	for( i = 0; i < 100; i++ )
	{
		char stp[1024];
		sprintf( stp, "%x", rand() );
		CNHashInsert( h, strdup(stp), strdup(stp) );
	}

	CNHashInsert( h, strdup("hello"), strdup("hello4") );
	CNHashInsert( h, strdup("hello"), strdup("hello1") );
	CNHashInsert( h, strdup("hello"), strdup("hello3") );
	CNHashInsert( h, strdup("hello"), strdup("hello2") );

	printf( "%s\n", (char*)CNHashGetValue( h,  "hello" ) );
	printf( "%p\n", (char*)CNHashGetValue( h,  "world" ) );

	CNHashDelete( h, "hello" );

	int nrvalues;
	void ** v = CNHashGetValueMultiple( h, "hello", &nrvalues );
	printf( "Vals: %d\n", nrvalues );
	for( i = 0; i < nrvalues; i++ )
	{
		printf( "%s\n", v[i] );
	}

	CNHashDestroy( h );
}

