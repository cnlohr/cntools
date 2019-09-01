#ifndef _CNRBTREE_H
#define _CNRBTREE_H

/*
	XXX TODO: Fix the delete stuff!!!
*/

#include <stdlib.h> //For malloc, free
#include <string.h> //For memset

//XXX TODO: Genericify more code.
//XXX TODO: Add size
//XXX TODO: Add total destruction.

#define CNRBTREE_COLOR_NONE  0
#define CNRBTREE_COLOR_RED   1
#define CNRBTREE_COLOR_BLACK 2

// Requires a temporary holders 'p' and 'g'
#define CNRBTREEPARENT( n )      ( p = (n)->parent )
#define CNRBTREEGRANDPARENT  ( ( p = CNRBTREEPARENT( n ) ), (p?PARENT(p):0) ) //CAREFUL: Do not call grandparent with p, it will break.
#define CNRBTREEUNCLE       ( ( g = GRANDPARENT( n ) ), g ? (SIBLING( p )) : 0 )

#define CNRBTREEGRANDPARENT_ ( g = (p?p->parent:0) )
#define CNRBTREEUNCLE_       ( g ? ( (p == g->left)?g->right:g->left ) : 0 )

//#ifdef CNRBTREE_IMPLEMENTATION

#ifndef CNRBTREE_GENERIC_DECORATOR
#define CNRBTREE_GENERIC_DECORATOR __attribute((weak))
#endif

struct cnrbtree_generic_node_t;
typedef struct cnrbtree_generic_node_t
{
	struct cnrbtree_generic_node_t * parent;
	char color;
	struct cnrbtree_generic_node_t * left;
	struct cnrbtree_generic_node_t * right;
} cnrbtree_generic_node;
typedef struct cnrbtree_generic_t
{
	struct cnrbtree_generic_node_t * node;
	//XXX TODO: add "size"
} cnrbtree_generic;

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateleft( cnrbtree_generic_node * n )
{ /* From Wikipedia's RB Tree Page */
	cnrbtree_generic_node * nnew = n->right;
	cnrbtree_generic_node * p = CNRBTREEPARENT(n);
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
	p = CNRBTREEPARENT(n);
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



/////////////////DELETION//////////////////

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_insert_repair_tree( cnrbtree_generic_node * n )
{
	cnrbtree_generic_node * p = CNRBTREEPARENT(n);
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
		cnrbtree_generic_node * u = CNRBTREEUNCLE_;
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
			if( g )	//XXX Why do we need to check?
			{
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
				p = n->parent;
				g = p->parent;
				p->color = CNRBTREE_COLOR_BLACK;
				if( g ) g->color = CNRBTREE_COLOR_RED; //XXX Why do we need to check?
			}
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







CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_replace_node( cnrbtree_generic_node * n, cnrbtree_generic_node * child )
{
	child->parent = n->parent;
	if (n == n->parent->left) {
		n->parent->left = child;
	} else {
		n->parent->right = child;
	}
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletecase1( cnrbtree_generic_node * n )
{
	cnrbtree_generic_node * p = n->parent;
	if( !p ) return;

	//DeleteCase2
	cnrbtree_generic_node * s = (p->left == n)?p->right:p->left; /* Sibling */
	printf( "n %p p %p [%p %p] s: %p\n", n, p, p->left, p->right, s );
	if( !s ) return;
	if (s->color == CNRBTREE_COLOR_RED)
	{
		p->color = CNRBTREE_COLOR_RED;
		s->color = CNRBTREE_COLOR_BLACK;
		if (n == p->left) {
			cnrbtree_generic_rotateleft(p);
		} else {
			cnrbtree_generic_rotateright(p);
		}
		printf( "p: %p n: %p n->p: %p   p->l: %p, p->r %p\n", p, n, n->parent, p->left, p->right );
		p = n->parent;
		printf( "p: %p n: %p n->p: %p   p->l: %p, p->r %p\n", p, n, n->parent, p->left, p->right );
		s = (p->left == n)?p->right:p->left; /* Update sibling */
	}

	//DeleteCase3
	if ((p->color == CNRBTREE_COLOR_BLACK) && (!s || s->color == CNRBTREE_COLOR_BLACK) &&
		(!s || !s->left || s->left->color == CNRBTREE_COLOR_BLACK) && (!s || !s->right || s->right->color == CNRBTREE_COLOR_BLACK))
	{
		s->color = CNRBTREE_COLOR_RED;
		cnrbtree_generic_deletecase1( p );
	}
	else
	{
		//DeleteCase4(n);
		printf( "{{ n: %p  p: %p s: %p\n", n, p, s );
		if ((p->color == CNRBTREE_COLOR_RED) && (!s || s->color == CNRBTREE_COLOR_BLACK) &&
			(!s || !s->left || s->left->color == CNRBTREE_COLOR_BLACK) && (!s || !s->right || s->right->color == CNRBTREE_COLOR_BLACK))
		{
			if( s ) s->color = CNRBTREE_COLOR_RED;
			p->color = CNRBTREE_COLOR_BLACK;
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
				}
				else if ((n == p->right) && (!s->left || s->left->color == CNRBTREE_COLOR_BLACK) &&
						   (s->right && s->right->color == CNRBTREE_COLOR_RED))
				{
						// This last test is trivial too due to cases 2-4.
						s->color = CNRBTREE_COLOR_RED;
						if(s->right ) s->right->color = CNRBTREE_COLOR_BLACK;
						cnrbtree_generic_rotateleft(s);
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
		}
	}
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletebase( cnrbtree_generic_node * n, cnrbtree_generic * t )
{
	if( !n->right && !n->left )
	{
		//Has no children
		cnrbtree_generic_deletecase1( n->parent ); //I don't know if this is correct.
		cnrbtree_generic_node * p = n->parent;
		if( p )
		{
			if( p->left == n ) p->left = 0;
			if( p->right == n ) p->right = 0;
		}
		while( p->parent ) p = p->parent;
		t->node = p;
	}
	else if( n->right && n->left )
	{
		//Standard delete; but we're only going to do left deletes.
		//XXX WARNING: I think this code produces a non-true red-black tree at worst case.  Not sure how to improve.

		int origcolor = n->color;
		cnrbtree_generic_node * p = n->parent;
		cnrbtree_generic_node * farright_in_left = n->left;

		while( farright_in_left->right ) farright_in_left = farright_in_left->right;

		//We have the far right node in our left subtree.  It does not have a right tree.
		//Detach its parent. If it's a leaf node, that's OK.
		if( farright_in_left->parent != n )
			farright_in_left->parent->right = farright_in_left->left;

		farright_in_left->left = n->left;
		farright_in_left->right = n->right;
		farright_in_left->color = origcolor;
		farright_in_left->parent = p;
		if( farright_in_left->right == farright_in_left ) farright_in_left->right = 0;
		if( farright_in_left->left == farright_in_left ) farright_in_left->left = 0;
		if( farright_in_left->right ) farright_in_left->right->parent = farright_in_left;
		if( farright_in_left->left ) farright_in_left->left->parent = farright_in_left;

		if( p != n->parent ) printf( "Warning: Check that should be deleted can't be deleted\n" );
		p = n->parent;

		if( p )
		{
			if( p->left == n ) p->left = farright_in_left;
			if( p->right == n ) p->right = farright_in_left;
		}

		while( farright_in_left->parent ) farright_in_left = farright_in_left->parent;
		t->node = farright_in_left;
	}
	else
	{
		//Exactly one child.

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

		//Child becomes the root.
		while( child->parent ) child = child->parent;
		t->node = child;
	}

	return;
}



#if 0

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_deletebase( cnrbtree_generic_node * n, cnrbtree_generic * t )
{
		//https://www.youtube.com/watch?v=CTvfzU_uNKE
	if( n->left && n->right ) //:27 
	{
		//Two not-null children.
		//Convert to case with 0 or 1 not null children.
	}

	//Check if red or child is red, then do replace :39
	cnrbtree_generic_node * child = n->left?n->left:n->right;
	
	//If we have a double-black case, we do this.
}

#endif







// #endif // CNRBTREE_IMPLEMENTATION

#define CNRBTREETEMPLATE( functiondecorator, key_t, data_t, comparexy, copykeyxy, deletekeyxy, doptrkey, doptrkeyinv, doptrdata, doptrdatainv ) \
	struct cnrbtree_##key_t##data_t##_node_t; \
	typedef struct cnrbtree_##key_t##data_t##_node_t \
	{ \
		struct cnrbtree_##key_t##data_t##_node_t * parent; \
		char color; \
		struct cnrbtree_##key_t##data_t##_node_t * left; \
		struct cnrbtree_##key_t##data_t##_node_t * right; \
		key_t key; \
		data_t data; \
	} cnrbtree_##key_t##data_t##_node; \
	typedef struct cnrbtree_##key_t##data_t##_t \
	{ \
		cnrbtree_##key_t##data_t##_node * node; \
	} cnrbtree_##key_t##data_t; \
	functiondecorator cnrbtree_##key_t##data_t * cnrbtree##key_t##data_t##create() \
	{\
		cnrbtree_##key_t##data_t * ret = malloc( sizeof( cnrbtree_##key_t##data_t ) ); \
		memset( ret, 0, sizeof( *ret ) ); \
		return ret; \
	}\
	functiondecorator cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##getltgt( cnrbtree_##key_t##data_t * tree, key_t doptrkey key, int lt, int gt ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = tree->node; \
		cnrbtree_##key_t##data_t##_node * tmpnext = tmp; \
		while( tmp ) \
		{ \
			int cmp = comparexy( key, doptrkeyinv tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( tmpnext == 0 ) \
			{ \
				if( lt && tmp->left ) return tmp->left; \
				if( gt && tmp->right ) return tmp->right; \
				break; \
			} \
			tmp = tmpnext; \
		} \
		return 0; \
	}\
	functiondecorator cnrbtree_##key_t##data_t##_node * cnrbtree##key_t##data_t##access( cnrbtree_##key_t##data_t * tree, key_t doptrkey key ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		cnrbtree_##key_t##data_t##_node * ret; \
		ret = malloc( sizeof( cnrbtree_##key_t##data_t##_node ) ); \
		memset( ret, 0, sizeof( *ret ) ); \
		copykeyxy( ret->key, key ); \
		ret->color = CNRBTREE_COLOR_RED; \
		/* Tricky shortcut for empty lists */ \
		if( tree->node == 0 ) \
		{ \
			ret->parent = 0; \
			ret->color = CNRBTREE_COLOR_BLACK; /* InsertCase1 */ \
			tree->node = ret; \
			goto skip; \
		 } \
		tmp = tree->node; \
		int cmp; \
		while( tmp ) \
		{ \
			cmp = comparexy( key, doptrkeyinv tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else return tmp; \
			if( !tmpnext ) break; \
			tmp = tmpnext; \
		} \
		ret->parent = tmp; \
		if( tmp ) \
			if( cmp < 0 ) tmp->left = ret; \
			else tmp->right = ret; \
		/* Here, [ret] is the new node, it's red, and [tmp] is our parent */ \
		if( tmp->color == CNRBTREE_COLOR_RED ) \
		{ \
			cnrbtree_generic_insert_repair_tree_with_fixup( (cnrbtree_generic_node*)ret, (cnrbtree_generic*)tree ); \
		} /* Else InsertCase2 */ \
		skip: return ret; \
	}\
	functiondecorator void cnrbtree##key_t##data_t##delete( cnrbtree_##key_t##data_t * tree, key_t doptrkey key ) \
	{\
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		cnrbtree_##key_t##data_t##_node * child; \
		if( tree->node == 0 ) return; \
		tmp = tree->node; \
		int cmp; \
		while( 1 ) \
		{ \
			cmp = comparexy( key, doptrkeyinv tmp->key ); \
			if( cmp < 0 ) tmpnext = tmp->left; \
			else if( cmp > 0 ) tmpnext = tmp->right; \
			else break; \
			if( !tmpnext ) return; \
			tmp = tmpnext; \
		} \
		/* found an item, tmp, to delete. */ \
		cnrbtree_generic_deletebase( (cnrbtree_generic_node*) tmp, (cnrbtree_generic*)tree ); \
		deletekeyxy( doptrkeyinv tmp->key, doptrdatainv tmp->data ); \
		free(tmp); \
	}

#endif

