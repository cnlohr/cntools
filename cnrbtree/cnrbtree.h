/* cnrbtree.h - v0.1pre - public domain C templated Red-Black Tree -
       https://github.com/cnlohr/cntools/cnrbtree
       no warranty implied; use at your own risk

   Do this:

     #define CNRBTREE_IMPLEMENTATION

   before you include this file in *one* C or C++ file to create the instance
   I.e. it should look something like this:

     #include <...>
     #include <...>
     #define CNRBTREE_IMPLEMENTATION
     #include "cnrbtree.h"

  Alternatively, if you are using a compile that supports the __weak__
  attribute you may just set CNRBTREE_GENERIC_DECORATOR and
  CNRBTREE_TEMPLATE_DECORATOR to __attribute__((weak))

  This is templated red-black tree for C.  It can be used for storing things
  like dictionaries, iterating through automatically sorting lists, finding
  elements in a dictionary that are equal to or greater than, or other similar
  unusual operations.

  Because this is templated, which means that for every combination of types,
  you can create the template which builds the functions  and types customized
  for your specific type.  This allows for a number of rather positive
  features:
    * The payload lives inside the nodes which are holding it.
      * Less malloc/free
      * Better cache coherency.
    * Avoid function calls for comparisons (great for primitive types).
    * Syntax for using the general accessor is very nice as it is automatically
      typed for your specific types.
    * Allows the compiler to apply extra optimization to your specific type.

  Usage: Somewhere in your program, before using a type, define the template:

     typedef struct object_t
     {
       int myvalue;
     } object;
     typedef char * str;
     CNRBTREETEMPLATE( str, int, RBstrcmp, RBstrcpy, RBstrdel );

  This will define a tree which uses strings to index.  You can then create
  these types.  I.e.

     //Constructs the tree
     cnrbtree_strint * tree = cnrbtree_strint_create();

     //Accesses, like C++ map's [] operator.
     RBA( tree, "a" ).myvalue = 5;
     RBA( tree, "d" ).myvalue = 8;
     RBA( tree, "c" ).myvalue = 7;
     RBA( tree, "b" ).myvalue = 6;

     //Access, like [] but reading.
     printf( "%d\n", RBA(tree, "c").myvalue );

     //Iterate through them all.
     RBFOREACH( str_payload, tree, i )
     {
         printf( ".key = %s .myvalue = %d\n", i->key, i-data.myvalue );
     }

     //Typesafe delete.
     RBDESTROY( tree );

  Authors:
    <>< Charles Lohr
 
  Based on Wikipedia article on red black trees.

  For judistictions where a public domain license is not available, the code
       may be licensed under:
   * New BSD (3-Clause) License
   * CC0 License
   * MIT/x11 License

  Version History:
     0.1pre - Initial Release (Incomplete and relatively slow)
*/

#ifndef _CNRBTREE_H
#define _CNRBTREE_H

//XXX TODO: Consider optimizations and pulling even more things out of the templated code into the regular code.


#if !defined( CNRBTREE_MALLOC ) || !defined( CNRBTREE_FREE )
#include <stdlib.h>
#endif

#ifndef CNRBTREE_MALLOC
#define CNRBTREE_MALLOC malloc
#endif

#ifndef CNRBTREE_FREE
#define CNRBTREE_FREE free
#endif

#ifndef CNRBTREE_MEMSET
#include <string.h> //For memset
#define CNRBTREE_MEMSET memset
#endif

#ifndef CNRBTREE_TEMPLATECODE
#define CNRBTREE_TEMPLATECODE 1
#endif


#ifndef CNRBTREE_GENERIC_DECORATOR
#define CNRBTREE_GENERIC_DECORATOR
#endif

#ifndef CNRBTREE_TEMPLATE_DECORATOR
#define CNRBTREE_TEMPLATE_DECORATOR
#endif


//Shorthand for red-black access, and typesafe deletion.
#ifndef NO_RBA
#define RBA(x,y) (x->access)( x, y )->data
#define RBDESTROY(x) (x->destroy)( x )
#define RBFOREACH( type, tree, i ) for( cnrbtree_##type##_node * i = tree->begin; i; i = (cnrbtree_##type##_node *)cnrbtree_generic_next( (cnrbtree_generic_node *)i ) )
#endif



struct cnrbtree_generic_node_t;
typedef struct cnrbtree_generic_node_t
{
	struct cnrbtree_generic_node_t * parent;
	struct cnrbtree_generic_node_t * left;
	struct cnrbtree_generic_node_t * right;
	char color;
} cnrbtree_generic_node;

struct cnrbtree_generic_t;
typedef struct cnrbtree_generic_t
{
	struct cnrbtree_generic_node_t * node;
	int size;
	cnrbtree_generic_node * (*access)( struct cnrbtree_generic_t * tree, void * key );
	void (*destroy)( struct cnrbtree_generic_t * tree );
	cnrbtree_generic_node * begin;
	cnrbtree_generic_node * tail;
} cnrbtree_generic;

#define CNRBTREE_COLOR_NONE  0
#define CNRBTREE_COLOR_RED   1
#define CNRBTREE_COLOR_BLACK 2




CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletebase( cnrbtree_generic_node * n, cnrbtree_generic * t );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic_node * node );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic_node * node );



#ifdef CNRBTREE_IMPLEMENTATION

CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic_node * node )
{
	if( !node ) return 0;
	if( node->right )
	{
		node = node->right;
		while( node->left ) node = node->left;
		return node;
	}
	if( node->parent && node == node->parent->left )
	{
		return node->parent;
	}
	while( node->parent && node->parent->right == node ) node = node->parent;
	return node->parent;
}


CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic_node * node )
{
	if( !node ) return 0;
	if( node->left )
	{
		node = node->left;
		while( node->right ) node = node->right;
		return node;
	}
	if( node->parent && node == node->parent->right )
	{
		return node->parent;
	}
	while( node->parent && node->parent->left == node ) node = node->parent;
	return node->parent;
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_update_begin_end( cnrbtree_generic * tree )
{
	cnrbtree_generic_node * tmp = tree->node;
	if( tmp )
	{
		while( tmp->left ) tmp = tmp->left;
	}
	tree->begin = tmp;
	tmp = tree->node;
	if( tmp )
	{
		while( tmp->right ) tmp = tmp->right;
	}
	tree->tail = tmp;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateleft( cnrbtree_generic_node * n )
{
	/* From Wikipedia's RB Tree Page */
	cnrbtree_generic_node * nnew = n->right;
	cnrbtree_generic_node * p = n->parent;
	n->right = nnew->left;
	nnew->left = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( n->right ) {
		n->right->parent = n;
	}
	/* Initially n could be the root. */
	if ( p ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateright( cnrbtree_generic_node * n )
{ /* From Wikipedia's RB Tree Page */
	cnrbtree_generic_node * nnew = n->left;
	cnrbtree_generic_node * p;
	p = n->parent;
	n->left = nnew->right;
	nnew->right = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( n->left ) {
		n->left->parent = n;
	}
	/* Initially n could be the root. */
	if ( p ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_insert_repair_tree( cnrbtree_generic_node * n )
{
	cnrbtree_generic_node * p = n->parent;
	if ( !p )
	{
		//InsertCase1(n); (In general, in this implementation, this block should not be hit)
		n->color = CNRBTREE_COLOR_BLACK;
	}
	else if ( p->color == CNRBTREE_COLOR_BLACK)
	{
		//InsertCase2(n);
		;
	}
	else
	{
		cnrbtree_generic_node * g = p->parent;
		cnrbtree_generic_node * u = ( g ? ( (p == g->left)?g->right:g->left ) : 0 );

		if ( u && u->color == CNRBTREE_COLOR_RED)
		{
			//InsertCase3(n);
			p->color = CNRBTREE_COLOR_BLACK;
			u->color = CNRBTREE_COLOR_BLACK;
			g->color = CNRBTREE_COLOR_RED;
			cnrbtree_generic_insert_repair_tree( g );
		}
		else
		{
			//InsertCase4(n)
			p = n->parent;
			g = p->parent;

			if ( n == p->right && p == g->left )
			{
				cnrbtree_generic_rotateleft(p);
				n = n->left;
			} else if ( n == p->left && p == g->right ) {
				cnrbtree_generic_rotateright(p);
				n = n->right;
			}

			//InsertCase4Step2(n);
			p = n->parent;
			g = p->parent;
			if (n == p->left) {
				cnrbtree_generic_rotateright(g);
			} else {
				cnrbtree_generic_rotateleft(g);
			}

			p->color = CNRBTREE_COLOR_BLACK;
			//if( g ) //Check probably not needed.
			g->color = CNRBTREE_COLOR_RED;
		}
	}
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_insert_repair_tree_with_fixup( cnrbtree_generic_node * n, cnrbtree_generic * tree )
{
	cnrbtree_generic_insert_repair_tree( n );

	/* Lastly, we must affix the root node's ptr correctly. */
	while( n->parent ) { n = n->parent; }
	tree->node = n;
}


CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc )
{
	cnrbtree_generic_node * ret;
	ret = CNRBTREE_MALLOC( sizetoalloc );
	CNRBTREE_MEMSET( ret, 0, sizeof( *ret ) );
	ret->color = CNRBTREE_COLOR_RED;

	/* Tricky shortcut for empty lists */
	tree->size++;
	if( tree->node == 0 )
	{
		ret->parent = 0;
		ret->color = CNRBTREE_COLOR_BLACK; /* InsertCase1 */
		tree->node = ret;
		cnrbtree_generic_update_begin_end( tree );
		return ret;
	}
	ret->parent = tmp;
	if( tmp )
	{
		if( cmp < 0 ) tmp->left = ret;
		else tmp->right = ret;
	}
	/* Here, [ret] is the new node, it's red, and [tmp] is our parent */ \
	if( tmp->color == CNRBTREE_COLOR_RED )
	{
		cnrbtree_generic_insert_repair_tree_with_fixup( (cnrbtree_generic_node*)ret, (cnrbtree_generic*)tree );
	} /* Else InsertCase2 */
	cnrbtree_generic_update_begin_end( tree );
	return ret;
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_exchange_nodes_internal( cnrbtree_generic_node * n1, cnrbtree_generic_node * n )
{

	{
		cnrbtree_generic_node tempexchange;
		tempexchange.parent = n->parent;
		tempexchange.left = n->left;
		tempexchange.right = n->right;
		tempexchange.color = n->color;

		n->parent = n1->parent;
		n->left = n1->left;
		n->right = n1->right;
		n->color = n1->color;

		n1->parent = tempexchange.parent;
		n1->left = tempexchange.left;
		n1->right = tempexchange.right;
		n1->color = tempexchange.color;
	}

	if( n1->parent == n1 ) n1->parent = n;
	if( n->parent == n ) n->parent = n1;

//I think this is OK, and automatically fixed up by the lower cases.
//	if( n1->left == n1 ) n1->left = n;
//	if( n->left == n ) n->left = n1;
//	if( n1->right == n1 ) n1->right = n;
//	if( n->right == n ) n->right = n1;

	if( n1->parent )
	{
		if( n1->parent->left == n ) n1->parent->left = n1;
		if( n1->parent->right == n ) n1->parent->right = n1;
	}
	if( n->parent )
	{
		if( n->parent->left == n1 ) n->parent->left = n;
		if( n->parent->right == n1 ) n->parent->right = n;
	}

	if( n1->right ) n1->right->parent = n1;
	if( n1->left ) n1->left->parent = n1;
	if( n->right ) n->right->parent = n;
	if( n->left ) n->left->parent = n;

}

/////////////////DELETION//////////////////



CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_replace_node( cnrbtree_generic_node * n, cnrbtree_generic_node * child )
{
	child->parent = n->parent;

	if( n->parent )
	{
		if (n == n->parent->left) {
			n->parent->left = child;
		} else {
			n->parent->right = child;
		}
	}
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletecase1( cnrbtree_generic_node * n )
{
	cnrbtree_generic_node temp_sibling;
	temp_sibling.left = 0;
	temp_sibling.right = 0;
	temp_sibling.color = CNRBTREE_COLOR_BLACK;
	temp_sibling.parent = 0;

	cnrbtree_generic_node * p = n->parent;

	if( !p ) return;

	if( n->color == CNRBTREE_COLOR_RED && !n->left && !n->right ) return;

	//DeleteCase2
	cnrbtree_generic_node * s = (p->left == n)?p->right:p->left; /* Sibling */
	if (s && s->color == CNRBTREE_COLOR_RED)
	{
		p->color = CNRBTREE_COLOR_RED;
		s->color = CNRBTREE_COLOR_BLACK;
		if (n == p->left) {
			cnrbtree_generic_rotateleft(p);
		} else {
			cnrbtree_generic_rotateright(p);
		}
		//p = n->parent; N's parent remains the same in this operation I think?
		s = (p->left == n)?p->right:p->left; //But the sibling updates.
	}

	//We can't have S be void, so we substitute in a temporary sibling.
	if( !s )
	{

		s = &temp_sibling;
		s->parent = p;
		if( s->parent->left == 0 ) s->parent->left = s;
		if( s->parent->right == 0 ) s->parent->right = s;
	}


	//DeleteCase3
	if ( (p->color == CNRBTREE_COLOR_BLACK) && ( s->color == CNRBTREE_COLOR_BLACK) &&
		( !s->left || s->left->color == CNRBTREE_COLOR_BLACK) && ( !s->right || s->right->color == CNRBTREE_COLOR_BLACK))
	{
		s->color = CNRBTREE_COLOR_RED;
		cnrbtree_generic_deletecase1( p );
		p = n->parent;
		s = (p->left == n)?p->right:p->left; 
	}
	else
	{
		//DeleteCase4(n);
		if (/* ( s != &temp_sibling ) &&*//* Probably unneeded */ (p->color == CNRBTREE_COLOR_RED) && (s->color == CNRBTREE_COLOR_BLACK) &&
			(!s->left || s->left->color == CNRBTREE_COLOR_BLACK) && (!s->right || s->right->color == CNRBTREE_COLOR_BLACK))
		{
			s->color = CNRBTREE_COLOR_RED;
			p->color = CNRBTREE_COLOR_BLACK;

			//Need to actually remove S. Probably unneeded.
			/*
			if( s == &temp_sibling )
			{
				if( s->parent->left == s ) s->parent->left = 0;
				if( s->parent->right == s ) s->parent->right = 0;
			}
			*/
		}
		else
		{
			//DeleteCase5(n);
			// This if statement is trivial, due to case 2 (even though case 2 changed
			// the sibling to a sibling's child, the sibling's child can't be red, since
			// no red parent can have a red child).
			if (s->color == CNRBTREE_COLOR_BLACK)
			{
				// The following statements just force the red to be on the left of the
				// left of the parent, or right of the right, so case six will rotate
				// correctly.
				if ((n == p->left) && (!s->right || s->right->color == CNRBTREE_COLOR_BLACK) &&
					(s->left && s->left->color == CNRBTREE_COLOR_RED))
				{
					// This last test is trivial too due to cases 2-4.
					s->color = CNRBTREE_COLOR_RED;
					if( s->left ) s->left->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateright(s);
					p = n->parent;
					s = (p->left == n)?p->right:p->left; 
				}
				else if ((n == p->right) && (!s->left || s->left->color == CNRBTREE_COLOR_BLACK) &&
						   (s->right && s->right->color == CNRBTREE_COLOR_RED))
				{
					// This last test is trivial too due to cases 2-4.
					s->color = CNRBTREE_COLOR_RED;
					if(s->right ) s->right->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateleft(s);
					p = n->parent;
					s = (p->left == n)?p->right:p->left; 
				}
			}

			//DeleteCase6(n);
			s->color = p->color;
			p->color = CNRBTREE_COLOR_BLACK;

			if (n == p->left) {
				if( s->right ) s->right->color = CNRBTREE_COLOR_BLACK;
				cnrbtree_generic_rotateleft( p );
			} else {
				if( s->left ) s->left->color = CNRBTREE_COLOR_BLACK;
				cnrbtree_generic_rotateright( p );
			}

			//Need to actually remove S. (CLEANUP)
			if( s == &temp_sibling )
			{
				if( s->parent->left == s ) s->parent->left = 0;
				if( s->parent->right == s ) s->parent->right = 0;
			}
		}
	}
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_delete_exactly_one( cnrbtree_generic_node * n, cnrbtree_generic * t )
{
	//DeleteOneChild (from the regular Wikipedia red black tree article)
	cnrbtree_generic_node * child = (!n->right) ? n->left : n->right;

	cnrbtree_generic_replace_node( n, child );
	if (n->color == CNRBTREE_COLOR_BLACK) {
		if (child->color == CNRBTREE_COLOR_RED) {
			child->color = CNRBTREE_COLOR_BLACK;
		} else {
			cnrbtree_generic_deletecase1(child);
		}
	}
	while( child->parent ) child = child->parent;
	t->node = child;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletebase( cnrbtree_generic_node * n, cnrbtree_generic * t )
{
	t->size--;
	if( !n->right && !n->left )
	{
		//Has no children
		cnrbtree_generic_deletecase1( n ); //I don't know if this is correct.
		cnrbtree_generic_node * p = n->parent;
		if( p )
		{
			if( p->left == n ) p->left = 0;
			if( p->right == n ) p->right = 0;
		}
		if( p )	while( p->parent ) p = p->parent;
		t->node = p;
	}
	else if( n->right && n->left )
	{
		int origcolor = n->color;
		cnrbtree_generic_node * p = n->parent;
		cnrbtree_generic_node * farright_in_left = n->left;
		while( farright_in_left->right ) farright_in_left = farright_in_left->right;

		cnrbtree_generic_exchange_nodes_internal( n, farright_in_left );

		cnrbtree_generic_deletecase1( n ); //I don't know if this is correct.
	//	cnrbtree_generic_delete_exactly_one( n, t );

		while( farright_in_left->parent ) farright_in_left = farright_in_left->parent;
		t->node = farright_in_left;

		if( n->parent->left == n ) n->parent->left = n->left;
		if( n->parent->right == n ) n->parent->right = n->left;

		//??? Not sure why I need to do this?
		if( n->left ) n->left->parent = n->parent;
		if( n->right ) n->left->parent = n->parent;
	}
	else
	{
		cnrbtree_generic_delete_exactly_one( n, t );
	}
	cnrbtree_generic_update_begin_end( t );
	return;
}

#endif // CNRBTREE_IMPLEMENTATION


//This is the template generator.  This is how new types are created.
#define CNRBTREETYPETEMPLATE( key_t, data_t ) \
	struct cnrbtree_##key_t##data_t##_node_t; \
	typedef struct cnrbtree_##key_t##data_t##_node_t \
	{ \
		struct cnrbtree_##key_t##data_t##_node_t * parent; \
		struct cnrbtree_##key_t##data_t##_node_t * left; \
		struct cnrbtree_##key_t##data_t##_node_t * right; \
		char color; \
		key_t key; \
		data_t data; \
	} cnrbtree_##key_t##data_t##_node; \
	struct cnrbtree_##key_t##data_t##_t; \
	typedef struct cnrbtree_##key_t##data_t##_t \
	{ \
		cnrbtree_##key_t##data_t##_node * node; \
		int size; \
		cnrbtree_##key_t##data_t##_node * (*access)( struct cnrbtree_##key_t##data_t##_t * tree, key_t key ); \
		void (*destroy)( struct cnrbtree_##key_t##data_t##_t * tree ); \
		cnrbtree_##key_t##data_t##_node * begin; \
		cnrbtree_##key_t##data_t##_node * tail; \
	} cnrbtree_##key_t##data_t; \
	\

#if CNRBTREE_TEMPLATECODE

#define CNRBTREETEMPLATE( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = tree->node; \
		cnrbtree_##key_t##data_t##_node * tmpnext = tmp; \
		while( tmp ) \
		{ \
			int cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( tmpnext == 0 ) \
			{ \
				return approx?tmp:0; \
			} \
			tmp = tmpnext; \
		} \
		return 0; \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		return cnrbtree_##key_t##data_t##_get2( tree, key, 0 ); \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_access( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		cnrbtree_##key_t##data_t##_node * ret; \
		tmp = tree->node; \
		int cmp = 0; \
		while( tmp ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( !tmpnext ) break; \
			tmp = tmpnext; \
		} \
		ret = (cnrbtree_##key_t##data_t##_node * ) cnrbtree_generic_insert_repair_tree_with_fixup_primary( \
			(cnrbtree_generic_node*)tmp, (cnrbtree_generic*)tree, \
			cmp, (int)sizeof( cnrbtree_##key_t##data_t##_node ) ); \
		copykeyxy( ret->key, key, ret->data ); \
		return ret; \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_delete( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		cnrbtree_##key_t##data_t##_node * child; \
		if( tree->node == 0 ) return; \
		tmp = tree->node; \
		int cmp; \
		while( 1 ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else break; \
			if( !tmpnext ) return; \
			tmp = tmpnext; \
		} \
		/* found an item, tmp, to delete. */ \
		cnrbtree_generic_deletebase( (cnrbtree_generic_node*) tmp, (cnrbtree_generic*)tree ); \
		deletekeyxy( tmp->key, tmp->data ); \
		CNRBTREE_FREE(tmp); \
	} \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t##_node * node ) \
	{\
		deletekeyxy( node->key, node->data ); \
		if( node->left ) cnrbtree_##key_t##data_t##_destroy_node_internal( node->left ); \
		if( node->right ) cnrbtree_##key_t##data_t##_destroy_node_internal( node->right ); \
		CNRBTREE_FREE( node ); \
	}\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ) \
	{\
		cnrbtree_##key_t##data_t##_destroy_node_internal( tree->node ); \
		CNRBTREE_FREE( tree ); \
	} \
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create() \
	{\
		cnrbtree_##key_t##data_t * ret = CNRBTREE_MALLOC( sizeof( cnrbtree_##key_t##data_t ) ); \
		CNRBTREE_MEMSET( ret, 0, sizeof( *ret ) ); \
		ret->access = cnrbtree_##key_t##data_t##_access; \
		ret->destroy = cnrbtree_##key_t##data_t##_destroy; \
		return ret; \
	}\

#else


#define CNRBTREETEMPLATE( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_access( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_delete( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t##_node * node ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create(); \

#endif


#define RBstrcmp(x,y) strcmp(x,y)
#define RBstrcpy(x,y,z) { x = strdup(y); }
#define RBstrdel(x,y) free( x );
#define RBCBSTR RBstrcmp, RBstrcpy, RBstrdel

#define RBptrcmp(x,y) (x-y)
#define RBptrcpy(x,y,z) { x = y; }
#define RBCBPTR RBptrcmp, RBptrcpy

#endif

