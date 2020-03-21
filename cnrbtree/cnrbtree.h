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
     CNRBTREETEMPLATE( str, object, RBstrcmp, RBstrcpy, RBstrdel );

  This will define a tree which uses strings to index.  You can then create
  these types.  I.e.

     //Constructs the tree
     cnrbtree_strint * tree = cnrbtree_strobject_create();

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
         printf( ".key = %s .myvalue = %d\n", i->key, i->data.myvalue );
     }

     //Typesafe delete.
     RBDESTROY( tree );

  You can also just do strint, and it will use strings as the index, and integers as the payload.

  Also provided: cnptrset - for a set-type data structure.  It is actually
     a cnrbtree_rbset_trbset_null_t and can be used as such.  This looks odd
     but, the size of the payload is 0 and because the types are templated
     the actual overhead for having a 0 payload is zero.

     cnptrset * set = cnptrset_create();
     static int var;
     cnptrset_insert( set, &var );
     void * i; //Quirk in cnptrset_foreach.  (TODO: can we remove that quirk?)
     cnptrset_foreach( set, i ) { printf( "%p\n", i ); }
     cnptrset_remove( set, &var );
     cnptrset_destroy( set );

  Authors:
    2019 <>< Charles Lohr
 
  Based on Wikipedia article on red black trees.

  For judistictions where a public domain license is not available, the code
       may be licensed under:
   * New BSD (3-Clause) License
   * CC0 License
   * MIT/x11 License

  Version History:
     0.1pre - Initial Release (Incomplete and relatively slow)


  Various design notes:

    MIT Press Introduction to Algorithms version, modified to use uncles, also
      a bit rearranged as an optimization.  Does not use recursion. Basically
      from CLRS 3rd Edition; It's based off of Cormen's algorithm.
    https://dpb.bitbucket.io/annotations-of-cormen-et-al-s-algorithm-for-a-red-black-tree-delete-and-delete-fixup-functions-only.html

    Major attractiveness:
    1) No recursion
    2) No special tail-recursion optimization required
         (which is MUCH slower on some compilers)
    3) No need to do complicated transplants/
    4) No need to copy data (this is evil if we're templating our types)

*/


#ifndef _CNRBTREE_H
#define _CNRBTREE_H

//XXX TODO: Consider optimizations and pulling even more things out of the templated code into the regular code.


#if !defined( CNRBTREE_MALLOC ) || !defined( CNRBTREE_FREE )
#include <stdlib.h>
#endif

//For creating trees and nodes
#ifndef CNRBTREE_MALLOC
#define CNRBTREE_MALLOC malloc
#endif

//For freeing trees and nodes.
#ifndef CNRBTREE_FREE
#define CNRBTREE_FREE free
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
#define RBHAS(x,y) ((x->get)( x, y ))
#define RBDESTROY(x) (x->destroy)( x )
#define RBFOREACH( type, tree, i ) for( cnrbtree_##type##_node * i = tree->begin; i != &tree->nil; i = (cnrbtree_##type##_node *)cnrbtree_generic_next( (cnrbtree_generic*)tree, (cnrbtree_generic_node *)i ) )
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
	cnrbtree_generic_node * (*get)( struct cnrbtree_generic_t * tree, void * key );
	void (*destroy)( struct cnrbtree_generic_t * tree );
	cnrbtree_generic_node * begin;
	cnrbtree_generic_node * tail;
	struct cnrbtree_generic_node_t nil;
} cnrbtree_generic;

//TODO: Consider collapsing this down to one bit.  Black == 0.
#define CNRBTREE_COLOR_NONE  0
#define CNRBTREE_COLOR_RED   1
#define CNRBTREE_COLOR_BLACK 2

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_removebase( cnrbtree_generic_node * n, cnrbtree_generic * t );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic *tree ,cnrbtree_generic_node * node );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic *tree ,cnrbtree_generic_node * node );

#ifdef CNRBTREE_IMPLEMENTATION

//The syntax used inside these functions is a little odd. It is
// written to help hint to the compiler what can be optimized.
// after significant testing, it seems to provide an edge over
// writing node->parent over and over when it should just be
// stored to a temporary register.
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic *tree, cnrbtree_generic_node * node )
{
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * tmp;
	if( node == nil ) return 0;
	tmp = node->right;
	if( tmp != nil )
	{
		node = tmp;
		while( (tmp = node->left), tmp != nil ) node = tmp;
		return node;
	}
	tmp = node->parent;
	if( tmp != nil && node == tmp->left )
	{
		return tmp;
	}
	while( tmp != nil && tmp->right == node ) { node = tmp; tmp = node->parent; }
	return tmp;
}


CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic * tree, cnrbtree_generic_node * node )
{
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * tmp;

	if( node == nil ) return 0;
	tmp = node->left;
	if( tmp != nil )
	{
		node = tmp;
		while( (tmp = node->right), tmp != nil ) node = tmp;
		return node;
	}
	tmp = node->parent;
	if( tmp != nil && node == tmp->right )
	{
		return tmp;
	}
	while( tmp != nil && tmp->left == node ) { node = tmp; tmp = node->parent; }
	return tmp;
}


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_update_begin_end( cnrbtree_generic * tree )
{
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * tmp = tree->node;
	if( tmp != nil )
	{
		while( tmp->left != nil ) tmp = tmp->left;
	}
	tree->begin = tmp;
	tmp = tree->node;
	if( tmp != nil )
	{
		while( tmp->right != nil ) tmp = tmp->right;
	}
	tree->tail = tmp;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateleft( cnrbtree_generic * tree, cnrbtree_generic_node * n )
{
	/* From Wikipedia's RB Tree Page, seems slightly better than the CLRS model, but now that it's been
		modified, there seems to be very little difference between them. */
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * nnew = n->right;
	cnrbtree_generic_node * p = n->parent;
	cnrbtree_generic_node * nright = n->right = nnew->left;
	nnew->left = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( nright != nil ) {
		nright->parent = n;
	}
	/* Initially n could be the root. */
	if ( p != nil ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateright( cnrbtree_generic * tree, cnrbtree_generic_node * n )
{
	/* From Wikipedia's RB Tree Page */
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * nnew = n->left;
	cnrbtree_generic_node * p = n->parent;
	cnrbtree_generic_node * nleft = n->left = nnew->right;
	nnew->right = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( nleft != nil ) {
		nleft->parent = n;
	}
	/* Initially n could be the root. */
	if ( p != nil ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_insert_repair_tree_with_fixup( cnrbtree_generic_node * z, cnrbtree_generic * tree  )
{
	cnrbtree_generic_node * nil = &tree->nil;
	cnrbtree_generic_node * zp;
	cnrbtree_generic_node * zpp;
	while( (( zp = z->parent ) != nil ) && zp->color == CNRBTREE_COLOR_RED )
	{
		zpp = zp->parent;

		cnrbtree_generic_node * u = (zp == zpp->left)?zpp->right:zpp->left;
		if( u->color == CNRBTREE_COLOR_RED )
		{
			//Case 1
			zp->color = CNRBTREE_COLOR_BLACK;
			u->color = CNRBTREE_COLOR_BLACK;
			zpp->color = CNRBTREE_COLOR_RED;
			z = zpp;
		}
		else
		{
			//Case 2 XXX Should we check NIL here?
			if( zp == zpp->left && z == zp->right )
			{
				cnrbtree_generic_rotateleft( tree, zp );
				z = zp;
				zp = z->parent;
				zpp = zp->parent;
			}
			else if( zp == zpp->right && z == zp->left )
			{
				cnrbtree_generic_rotateright( tree, zp );
				z = zp;
				zp = z->parent;
				zpp = zp->parent;
			}

			zp->color = CNRBTREE_COLOR_BLACK;

			//Case 3
			if( zpp != nil )
			{
				zpp->color = CNRBTREE_COLOR_RED;
				if( zpp->left == zp )
					cnrbtree_generic_rotateright( tree, zpp );
				else
					cnrbtree_generic_rotateleft( tree, zpp );
			}
		}
	}

	// Lastly, we must affix the root node's ptr correctly.
	while( (zp = z->parent), zp != nil ) { z = zp; }
	tree->node = z;
}

CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc )
{
	cnrbtree_generic_node * ret;
	cnrbtree_generic_node * nil = &tree->nil;
	ret = CNRBTREE_MALLOC( sizetoalloc );

	ret->color = CNRBTREE_COLOR_RED;
	ret->left = &tree->nil;
	ret->right = &tree->nil;
	ret->parent = &tree->nil;

	/* Tricky shortcut for empty lists */
	tree->size++;
	if( tree->node == nil )
	{
		ret->parent = nil;
		ret->color = CNRBTREE_COLOR_BLACK; /* InsertCase1 from wikipedia */
		tree->node = ret;
		cnrbtree_generic_update_begin_end( tree );
		return ret;
	}
	ret->parent = tmp;

	//XXX Should we protect 'tmp' to make sure it's not RBNIL?
	if( cmp < 0 ) tmp->left = ret;
	else tmp->right = ret;

	/* Here, [ret] is the new node, it's red, and [tmp] is our parent */ \
	if( tmp->color == CNRBTREE_COLOR_RED )
	{
		cnrbtree_generic_insert_repair_tree_with_fixup( (cnrbtree_generic_node*)ret, (cnrbtree_generic*)tree );
	} /* Else InsertCase2 */
	cnrbtree_generic_update_begin_end( tree );
	return ret;
}

/////////////////DELETION//////////////////


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_transplant( cnrbtree_generic * T, cnrbtree_generic_node * u, cnrbtree_generic_node * v )
{
	cnrbtree_generic_node * nil = &T->nil;
	cnrbtree_generic_node * up = u->parent;
	if( up == nil )
		T->node = v;
	else if( u == up->left )
		up->left = v;
	else
		up->right = v;
	v->parent = u->parent; //Not sure what algorithm witchcraft is going on here, but everything breaks if you "protect" this from NIL.
}

//"RB-DELETE(T, z)"
CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_removebase( cnrbtree_generic_node * z, cnrbtree_generic * T )
{
	T->size--;

	cnrbtree_generic_node * nil = &T->nil;
	cnrbtree_generic_node * x;
	cnrbtree_generic_node * y = z;
	char y_original_color = y->color;

	if( z->left == nil )
	{
		x = z->right;
		cnrbtree_generic_transplant( T, z, x );
	}
	else if( z->right == nil )
	{
		x = z->left;
		cnrbtree_generic_transplant( T, z, x );		
	}
	else
	{
		// XXX How is it possible that this never fails?! I would have expected to need to check if nil and if so, do cnrbtree_generic_prev.
		y = cnrbtree_generic_next( T, z ); 

		y_original_color = y->color;
		cnrbtree_generic_node * tmp = y->right;

		x = tmp; //I would be concerned if X is nil, but that appears to be OK.

		if( y->parent == z )
		{
			x->parent = y;
		}
		else
		{
			cnrbtree_generic_transplant( T, y, tmp );
			tmp = y->right = z->right;
			tmp->parent = y;
		}

		cnrbtree_generic_transplant( T, z, y );

		tmp = y->left = z->left;
		tmp->parent = y;
		y->color = z->color;
	}
	if( y_original_color == CNRBTREE_COLOR_BLACK )
	{
		//"RB-DELETE-FIXUP( T,x )"
		cnrbtree_generic_node * w;
		cnrbtree_generic_node * xp;

		while( x->color == CNRBTREE_COLOR_BLACK )
		{
			xp = x->parent;
			if( x == xp->left )
			{
				w = xp->right;
				if( w->color == CNRBTREE_COLOR_RED )
				{
					w->color = CNRBTREE_COLOR_BLACK;
					xp->color = CNRBTREE_COLOR_RED;
					cnrbtree_generic_rotateleft( T, xp );
					w = xp->right;
				}
				if( ( w->left->color == CNRBTREE_COLOR_BLACK ) && 
					( w->right->color == CNRBTREE_COLOR_BLACK ) )
				{
					w->color = CNRBTREE_COLOR_RED;
					x = xp;
				}
				else
				{
					if( w->right->color == CNRBTREE_COLOR_BLACK )
					{
						w->left->color = CNRBTREE_COLOR_BLACK;
						w->color = CNRBTREE_COLOR_RED;
						cnrbtree_generic_rotateright( T, w );
						w = xp->right;
					}
					w->color = x->parent->color;
					xp->color = CNRBTREE_COLOR_BLACK;
					w->right->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateleft( T, xp );
					break;
				}
			}
			else
			{
				//Same as above but inverted sides.
				w = xp->left;
				if( w->color == CNRBTREE_COLOR_RED )
				{
					w->color = CNRBTREE_COLOR_BLACK;
					xp->color = CNRBTREE_COLOR_RED;
					cnrbtree_generic_rotateright( T, xp );
					w = xp->left;
				}
				if( ( w->right->color == CNRBTREE_COLOR_BLACK ) && 
					( w->left->color == CNRBTREE_COLOR_BLACK ) )
				{
					w->color = CNRBTREE_COLOR_RED;
					x = xp;
				}
				else
				{
					if( w->left->color == CNRBTREE_COLOR_BLACK )
					{
						w->right->color = CNRBTREE_COLOR_BLACK;
						w->color = CNRBTREE_COLOR_RED;
						cnrbtree_generic_rotateleft( T, w );
						w = xp->left;
					}
					w->color = xp->color;
					x->parent->color = CNRBTREE_COLOR_BLACK;
					w->left->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateright( T, xp );
					break;
				}
			}
		}

		x->color = CNRBTREE_COLOR_BLACK;

		// We must affix the root node's ptr correctly.
		while( (xp = x->parent), xp != nil ) { x = xp; }
		T->node = x;

		//End "RB-DELETE-FIXUP( T,x )"
	}

	if( T->size == 0 ) { 
		T->node = nil;
	}

	cnrbtree_generic_update_begin_end( T );
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
		cnrbtree_##key_t##data_t##_node * (*get)( struct cnrbtree_##key_t##data_t##_t * tree, key_t key ); \
		void (*destroy)( struct cnrbtree_##key_t##data_t##_t * tree ); \
		cnrbtree_##key_t##data_t##_node * begin; \
		cnrbtree_##key_t##data_t##_node * tail; \
		cnrbtree_##key_t##data_t##_node nil; \
	} cnrbtree_##key_t##data_t; \
	\

#if CNRBTREE_TEMPLATECODE

#define CNRBTREETEMPLATE( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = &tree->nil; \
		cnrbtree_##key_t##data_t##_node * tmp = tree->node; \
		cnrbtree_##key_t##data_t##_node * tmpnext = tmp; \
		while( tmp != nil ) \
		{ \
			int cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( tmpnext == nil ) \
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
		cnrbtree_##key_t##data_t##_node * nil = &tree->nil; \
		/* This function could utilize cnrbtree_##key_t##data_t##_get2 but would require an extra compare */ \
		cnrbtree_##key_t##data_t##_node * tmp = 0;  \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		tmp = tree->node; \
		int cmp = 0; \
		while( tmp != nil ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( tmpnext == nil ) break; \
			tmp = tmpnext; \
		} \
		cnrbtree_##key_t##data_t##_node * ret; \
		ret = (cnrbtree_##key_t##data_t##_node * ) cnrbtree_generic_insert_repair_tree_with_fixup_primary( \
			(cnrbtree_generic_node*)tmp, (cnrbtree_generic*)tree, \
			cmp, (int)sizeof( cnrbtree_##key_t##data_t##_node ) ); \
		copykeyxy( ret->key, key, ret->data ); \
		return ret; \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_remove( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = &tree->nil; \
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		cnrbtree_##key_t##data_t##_node * child; \
		if( tree->node == nil ) return; \
		tmp = tree->node; \
		int cmp; \
		while( 1 ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else break; \
			if( tmpnext == nil ) return; \
			tmp = tmpnext; \
		} \
		/* found an item, tmp, to delete. */ \
		cnrbtree_generic_removebase( (cnrbtree_generic_node*) tmp, (cnrbtree_generic*)tree ); \
		deletekeyxy( tmp->key, tmp->data ); \
		CNRBTREE_FREE(tmp); \
	} \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t * tree, cnrbtree_##key_t##data_t##_node * node ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = &tree->nil; \
		if( node == nil ) return; \
		deletekeyxy( node->key, node->data ); \
		if( node->left != nil ) cnrbtree_##key_t##data_t##_destroy_node_internal( tree, node->left ); \
		if( node->right != nil ) cnrbtree_##key_t##data_t##_destroy_node_internal( tree, node->right ); \
		CNRBTREE_FREE( node ); \
	}\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ) \
	{\
		cnrbtree_##key_t##data_t##_destroy_node_internal( tree, tree->node ); \
		CNRBTREE_FREE( tree ); \
	} \
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create() \
	{\
		cnrbtree_##key_t##data_t * ret = CNRBTREE_MALLOC( sizeof( cnrbtree_##key_t##data_t ) ); \
		ret->node = (cnrbtree_##key_t##data_t##_node *)&ret->nil; \
		ret->tail = (cnrbtree_##key_t##data_t##_node *)&ret->nil; \
		ret->begin = (cnrbtree_##key_t##data_t##_node *)&ret->nil; \
		ret->size = 0; \
		ret->access = cnrbtree_##key_t##data_t##_access; \
		ret->get = cnrbtree_##key_t##data_t##_get; \
		ret->destroy = cnrbtree_##key_t##data_t##_destroy; \
		ret->nil.parent = &ret->nil; \
		ret->nil.left   = &ret->nil; \
		ret->nil.right  = &ret->nil; \
		ret->nil.color  = CNRBTREE_COLOR_BLACK; \
		return ret; \
	}\

#else
#define CNRBTREETEMPLATE CNRBTREETEMPLATE_DEFINITION
#endif


#define CNRBTREETEMPLATE_DEFINITION( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_access( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_remove( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t * tree, cnrbtree_##key_t##data_t##_node * node ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create(); \

#define RBstrcmp(x,y) strcmp(x,y)
#define RBstrcpy(x,y,z) { x = strdup(y); }
#define RBstrdel(x,y) free( x );
#define RBCBSTR RBstrcmp, RBstrcpy, RBstrdel

#define RBptrcmp(x,y) ((x==y)?0:((((intptr_t)x-(intptr_t)y)<0)?-1:1))
#define RBptrcpy(x,y,z) { x = y; }
#define RBnullop(x,y)
#define RBCBPTR RBptrcmp, RBptrcpy

#ifndef CNRBTREE_NO_SETTYPES

#include <string.h>

//Code for pointer-sets (cnptrset) - this is only for void *
typedef void * rbset_t;
typedef char rbset_null_t[0];
#ifdef CNRBTREE_IMPLEMENTATION
	CNRBTREETEMPLATE( rbset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBnullop );
#else
	CNRBTREETEMPLATE_DEFINITION( rbset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBnullop );
#endif

typedef cnrbtree_rbset_trbset_null_t cnptrset;
#define cnptrset_create() cnrbtree_rbset_trbset_null_t_create()
#define cnptrset_insert( st, key ) cnrbtree_rbset_trbset_null_t_access( st, key )
#define cnptrset_remove( st, key ) cnrbtree_rbset_trbset_null_t_remove( st, key )
#define cnptrset_destroy( st ) cnrbtree_rbset_trbset_null_t_destroy( st )
//Note, you need a pre-defined void * for the type in the iteration. i.e. void * i; cnptrfset_foreach( tree, i );
#define cnptrset_foreach( tree, i ) \
	for( cnrbtree_rbset_trbset_null_t_node * node##i = tree->begin; \
		i = (node##i)->key, node##i != &tree->nil; \
		node##i = (cnrbtree_rbset_trbset_null_t_node *)cnrbtree_generic_next( (cnrbtree_generic*)tree, (cnrbtree_generic_node *)node##i ) )



//Code for string-sets (cnstrset) - this is only for void *
typedef char * rbstrset_t;
#ifdef CNRBTREE_IMPLEMENTATION
	CNRBTREETEMPLATE( rbstrset_t, rbset_null_t, RBstrcmp, RBstrcpy, RBstrdel );
#else
	CNRBTREETEMPLATE_DEFINITION( rbstrset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBstrdel );
#endif

typedef cnrbtree_rbstrset_trbset_null_t cnstrset;
#define cnstrset_create() cnrbtree_rbstrset_trbset_null_t_create()
#define cnstrset_insert( st, key ) cnrbtree_rbstrset_trbset_null_t_access( st, key )
#define cnstrset_remove( st, key ) cnrbtree_rbstrset_trbset_null_t_remove( st, key )
#define cnstrset_destroy( st ) cnrbtree_rbstrset_trbset_null_t_destroy( st )
//Note, you need a pre-defined char * for the type in the iteration. i.e. char * i; cnstrfset_foreach( tree, i );
#define cnstrset_foreach( tree, i ) \
	for( cnrbtree_rbstrset_trbset_null_t_node * node##i = tree->begin; \
		i = (node##i)->key, node##i != &tree->nil; \
		node##i = (cnrbtree_rbstrset_trbset_null_t_node *)cnrbtree_generic_next( (cnrbtree_generic*)tree, (cnrbtree_generic_node *)node##i ) )

#endif

#endif

