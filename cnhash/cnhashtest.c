#include "cnhash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main()
{
	int i;
	cnhashtable * h = CNHashGenerate( 1, 0, CNHASH_STRINGS );
	CNHashInsert( h, strdup("hello"), strdup("helloX") );

#if 1
	for( i = 0; i < 1000; i++ )
	{
		char stp[1024];
		sprintf( stp, "%x", rand() );
		CNHashInsert( h, strdup(stp), strdup(stp) );
	}
#endif
	CNHashInsert( h, strdup("hello"), strdup("hello4") );
	CNHashInsert( h, strdup("hello"), strdup("hello1") );
	CNHashInsert( h, strdup("hello"), strdup("hello3") );
	CNHashInsert( h, strdup("hello"), strdup("hello2") );

	printf( "%s\n", (char*)CNHashGetValue( h,  "hello" ) );
	printf( "%p\n", (char*)CNHashGetValue( h,  "world" ) );


	int nrvalues;
	void ** v = CNHashGetValueMultiple( h, "hello", &nrvalues );
	printf( "Vals: %d\n", nrvalues );
	for( i = 0; i < nrvalues; i++ )
	{
		printf( "%s\n", (char*)v[i] );
	}

	//CNHashDump( h );
	CNHashDelete( h, "hello" );
	//CNHashDump( h );

	v = CNHashGetValueMultiple( h, "hello", &nrvalues );
	printf( "Vals: %d\n", nrvalues );
	for( i = 0; i < nrvalues; i++ )
	{
		printf( "%s\n", (char*)v[i] );
	}

	CNHashDestroy( h );
}

