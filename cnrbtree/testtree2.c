#include <stdlib.h>
#include <stdio.h>
#include "cnrbtree.h"

#define tstrcopy(x,y,z) { x = strdup(y); z.a = 0; z.b = 0; }
#define tstrdelete(x,y) free( x );

typedef struct some_payload_t
{
	int a;
	int b;
} payload;
typedef char * str_;

CNRBTREETEMPLATE( str_ , payload, RBstrcmp, tstrcopy, RBstrdel );
CNRBTREETEMPLATE( str_ , int, RBstrcmp, RBstrcpy, tstrdelete );

int anotherfn()
{
	cnrbtree_str_payload  * tree = cnrbtree_str_payload_create();
	cnrbtree_str_int      * treesi = cnrbtree_str_int_create();

	RBA( tree, "hello" ).a = 5;
	RBA( tree, "world" ).b = 6;
	RBA( tree, "how" ).a = 9;
	RBA( tree, "are" ).b = 4;
	RBA( tree, "you" ).a = 3;
	RBA( tree, "a" );
	RBA( tree, "b" );
	RBA( tree, "c" );
	RBA( tree, "d" );
	RBA( tree, "e" );
	RBA( tree, "f" );
	RBA( tree, "g" );
	RBA( tree, "h" );

	printf( "%d\n", RBA( tree, "hello" ).a );
	printf( "%d\n", RBA( tree, "how" ).a );
	printf( "%d\n", RBA( tree, "are" ).b );
	printf( "%d\n", RBA( tree, "you" ).a );
	printf( "%d\n", RBA( tree, "world" ).b );

	cnrbtree_str_payload_remove( tree, "are" );

	RBFOREACH( str_payload, tree, i )
	{
		printf( ".key = %5s   .a = %d .b = %d\n", i->key, i->data.a, i->data.b );
	}

	RBDESTROY( tree );
	return 0;
}
