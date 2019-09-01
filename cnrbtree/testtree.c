#include <stdio.h>
#include "cnrbtree.h"

#define intcomp(x, y) ( x - y )
#define intcopy(x, y) (x = y)
#define intdelete( x, y )

CNRBTREETEMPLATE( , int, int, intcomp, intcopy, intdelete, , , , );

void PrintTree( cnrbtree_intint_node * t, int depth )
{
	if( !t )
	{
		printf( "%*s-\n", depth*4, "" );
		return;
	}
	printf( "%*s%d %d (%p) PARENT: %p\n", depth*4, "", t->key, t->color, t, t->parent );
	PrintTree( t->left, depth+1 );
	PrintTree( t->right, depth+1 );
}

int main()
{
	cnrbtree_intint * tree;
	tree = cnrbtreeintintcreate();
#if 0
	printf( "Created\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 1 )->data = 10;
	printf( "Inserted1\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 3 )->data = 30;
	printf( "Inserted3\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 5 )->data = 50;
	printf( "Inserted5\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 4 )->data = 40;
	printf( "Inserted4\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 2 )->data = 20;
	printf( "Inserted2\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 6 )->data = 60;
	printf( "Inserted6\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 7 )->data = 70;
	printf( "Inserted7\n" );
	PrintTree( tree->node, 0 );
	cnrbtreeintintaccess( tree, 8 )->data = 80;
	printf( "Inserted7\n" );
	PrintTree( tree->node, 0 );
#endif

	int addlist[100];
	int i;
	for( i = 0; i < 100; i++ )
	{
		PrintTree( tree->node, 0 );
		printf( "Adding: %d\n", (addlist[i] = rand()) );
		cnrbtreeintintaccess( tree, addlist[i] )->data = 80;
	}

	for( i = 0; i < 100; i++ )
	{
		int k = addlist[i];
		printf( "Deleting %d (%d)\n", k, i );
		cnrbtreeintintdelete( tree, k );
		printf( "Deleted %d\n", k );
		PrintTree( tree->node, 0 );
	}

/*
	cnrbtreeintintaccess( tree, 50 )->data = 80;
	cnrbtreeintintaccess( tree, 60 )->data = 80;
	cnrbtreeintintaccess( tree, 70 )->data = 80;
	cnrbtreeintintaccess( tree, 80 )->data = 80;
	cnrbtreeintintaccess( tree, 40 )->data = 80;
	cnrbtreeintintaccess( tree, 30 )->data = 80;
	cnrbtreeintintaccess( tree, 20 )->data = 80;
	cnrbtreeintintaccess( tree, 10 )->data = 80;
	cnrbtreeintintaccess( tree, 5 )->data = 80;
	PrintTree( tree->node, 0 );


	cnrbtree_intint_node * r = cnrbtree_intintgetltgt( tree, 5, 0, 0 );
	printf( "%p\n", r );
	printf( "%d\n", r->data );

	cnrbtreeintintdelete( tree, 30 );
	printf( "Deleted 30\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 50 );
	printf( "Deleted 50\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 60 );
	printf( "Deleted 60\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 80 );
	printf( "Deleted 60\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 70 );
	printf( "Deleted 60\n" );
	PrintTree( tree->node, 0 );
*/

#if 0
	cnrbtreeintintdelete( tree, 60 );
	printf( "Deleted 60\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 50 );
	printf( "Deleted 50\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 70 );
	printf( "Deleted 70\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 20 );
	printf( "Deleted 20\n" );
	PrintTree( tree->node, 0 );

	cnrbtreeintintdelete( tree, 80 );
	printf( "Deleted 80\n" );
	PrintTree( tree->node, 0 );
#endif

}

