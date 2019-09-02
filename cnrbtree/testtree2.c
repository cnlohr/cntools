#include <stdlib.h>
#include <stdio.h>
#include "cnrbtree.h"

#define tstrcomp(x,y) strcmp(x,y)
#define tstrcopy(x,y) x = strdup(y);
#define tstrdelete(x,y) free( x );

//typedef char * ᚜str᚛;
//typedef int    ᚜int᚛; 

typedef struct some_payload_t
{
	int a;
	int b;
} payload;
typedef char * str_;
CNRBTREETEMPLATE( , str_ , payload, tstrcomp, tstrcopy, tstrdelete, , , , );

int PrintTreeRootIt( struct cnrbtree_generic_node_t * t );

int anotherfn()
{
	cnrbtree_str_payload  * tree = cnrbtree_str_payload_create();
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

	FOREACH_IN_TREE_TYPE( str_payload, tree, i )
	{
		printf( ".key = %5s   .a = %d .b = %d\n", i->key, i->data.a, i->data.b );
	}

	RBDESTROY( tree );
	return 0;
}
