struct cnrbtree_generic_node_t;
int PrintTreeRootIt( struct cnrbtree_generic_node_t * t );

#define CNRBTREE_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include "cnrbtree.h"

int gints = 0;

#define intcomp(x, y) ( x - y )
#define intcopy(x, y) (x = y, gints++ ) 
#define intdelete( x, y )     gints--;

CNRBTREETEMPLATE( , int, int, intcomp, intcopy, intdelete, , , , );

int PrintTree( cnrbtree_intint_node * t, int depth, cnrbtree_intint_node * parent );

int PrintTreeRootIt( cnrbtree_generic_node * t )
{
	return 0;
	while( t->parent ) t = t->parent;
	PrintTree( (cnrbtree_intint_node *)t, -10000, 0 );
	return 0;
}


int PrintTree( cnrbtree_intint_node * t, int depth, cnrbtree_intint_node * parent )
{
	return 0;
	int stordepth = depth;
	if( depth < 0 ) depth += 10000;

	if( !t )
	{
		printf( "%*s-\n", depth*4, "" );
		return 1;
	}
	printf( "%*s%d %d (%p) PARENT: %p\n", depth*4, "", t->key, t->color, t, t->parent );
	int d1 = PrintTree( t->left, stordepth+1, t );
	int d2 = PrintTree( t->right, stordepth+1, t );
	if( d1 != d2 && stordepth >= 0 )
	{
		fprintf( stderr, "Black Fault (%d != %d)\n", d1, d2 );
		exit(1);
	}
	if( stordepth >= 0 && t->color == CNRBTREE_COLOR_RED && t->parent && t->parent->color == CNRBTREE_COLOR_RED )
	{
		fprintf( stderr, "Red Fault\n" );
		exit(1);
	}
	if( stordepth >= 0 && t->parent != parent )
	{
		fprintf( stderr, "Parent fault\n" );
		exit( 1);
	}
	return (depth < 0)?-1:(d1 + (t->color == CNRBTREE_COLOR_BLACK));
	/*
		Every node is either red or black.
		Every leaf (NULL) is black.
		If a node is red, then both its children are black.
		Every simple path from a node to a descendant leaf contains the same number of black nodes. 
	*/
}

int main()
{
	cnrbtree_intint * tree;
	tree = cnrbtree_intint_create();

#if 1
	srand(0);
	#define ITERATIONS 1000
	int addlist[ITERATIONS];
	int i;
	int j;
	for( j = 0; j < 3000; j++ )
	{
		for( i = 0; i < ITERATIONS; i++ )
		{
			PrintTree( tree->node, 0, 0 );
	retry:
			printf( "Adding: %d\n", (addlist[i] = rand()) );
			if( cnrbtree_intint_getltgt( tree, addlist[i], 0, 0 ) )
			{
				printf( "Duplicate.  Try again.\n" );
				goto retry;
			}


			cnrbtree_intint_access( tree, addlist[i] )->data = 80;
			printf( "SIZE: %d\n", tree->size );
		}

		if( tree->size != ITERATIONS)
		{
			printf( "Size violation. %d\n", tree->size );
			exit( 5 );
		}
		PrintTree( tree->node, 0, 0 );

		for( i = 0; i < ITERATIONS; i++ )
		{
			int k = addlist[i];
			printf( "Deleting %d (%d)\n", k, i );
			if( !cnrbtree_intint_getltgt( tree, addlist[i], 0, 0 ) )
			{
				printf( "Access fault.\n" );
				exit( 5 );
			}
			cnrbtree_intint_delete( tree, k );
			printf( "Deleted %d\n", k );
			PrintTree( tree->node, 0,0 );
		}
		if( tree->node )
		{
			printf( "Excess fault\n" );
			exit( 6 );
		}
		if( tree->size != 0 )
		{
			printf( "Size violation. %d\n", tree->size );
			exit( 5 );
		}

	}



	for( i = 0; i < 100; i++ )
	{
		PrintTree( tree->node, 0, 0 );
retry2:
		printf( "Adding: %d\n", (addlist[i] = rand()) );
		if( cnrbtree_intint_getltgt( tree, addlist[i], 0, 0 ) )
		{
			printf( "Duplicate.  Try again.\n" );
			goto retry2;
		}


		cnrbtree_intint_access( tree, addlist[i] )->data = 80;
		printf( "SIZE: %d\n", tree->size );
	}

	cnrbtree_intint_destroy( tree );

	if( gints != 0 )
	{
		printf( "gints fault\n" );
		exit( -6 );
	}
#endif
int anotherfn();
	anotherfn();
	fprintf( stderr, "OK\n" );


}

