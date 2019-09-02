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
	struct cnrbtree_generic_node_t * left;
	struct cnrbtree_generic_node_t * right;
	char color;
} cnrbtree_generic_node;
typedef struct cnrbtree_generic_t
{
	struct cnrbtree_generic_node_t * node;
	int size;
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
			if( g ) g->color = CNRBTREE_COLOR_RED; //XXX Why do we need to check?
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


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_exchange_nodes_internal( cnrbtree_generic_node * n1, cnrbtree_generic_node * n )
{

	{
		cnrbtree_generic_node tempexchange;
		tempexchange.left = n->left;
		tempexchange.right = n->right;
		tempexchange.color = n->color;
		tempexchange.parent = n->parent;

		n->color = n1->color;
		n->parent = n1->parent;
		n->left = n1->left;
		n->right = n1->right;

		n1->color = tempexchange.color;
		n1->parent = tempexchange.parent;
		n1->left = tempexchange.left;
		n1->right = tempexchange.right;
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
	PrintTreeRootIt( n );
	if (s && s->color == CNRBTREE_COLOR_RED)
	{
		p->color = CNRBTREE_COLOR_RED;
		s->color = CNRBTREE_COLOR_BLACK;
		if (n == p->left) {
			cnrbtree_generic_rotateleft(p);
		} else {
			cnrbtree_generic_rotateright(p);
		}
		//printf( "p: %p n: %p n->p: %p   p->l: %p, p->r %p\n", p, n, n->parent, p->left, p->right );
		//p = n->parent; N's parent remains the same in this operation I think?
		s = (p->left == n)?p->right:p->left; //But the sibling updates.
	}


	//Tricky! We can't have S be void.
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
		if ( ( s != &temp_sibling ) /* !!?@?@!? XXX Do we want this test?!? */ && (p->color == CNRBTREE_COLOR_RED) && (s->color == CNRBTREE_COLOR_BLACK) &&
			(!s->left || s->left->color == CNRBTREE_COLOR_BLACK) && (!s->right || s->right->color == CNRBTREE_COLOR_BLACK))
		{
			s->color = CNRBTREE_COLOR_RED;
			p->color = CNRBTREE_COLOR_BLACK;

			//Need to actually remove S.
			if( s == &temp_sibling )
			{
				if( s->parent->left == s ) s->parent->left = 0;
				if( s->parent->right == s ) s->parent->right = 0;
			}
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
	//	cnrbtree_generic_deletebase( n,t );

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

	return;
}

















// #endif // CNRBTREE_IMPLEMENTATION

#define CNRBTREETEMPLATE( functiondecorator, key_t, data_t, comparexy, copykeyxy, deletekeyxy, doptrkey, doptrkeyinv, doptrdata, doptrdatainv ) \
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
	typedef struct cnrbtree_##key_t##data_t##_t \
	{ \
		cnrbtree_##key_t##data_t##_node * node; \
		int size; \
	} cnrbtree_##key_t##data_t; \
	functiondecorator cnrbtree_##key_t##data_t * cnrbtree##key_t##data_t##create() \
	{\
		cnrbtree_##key_t##data_t * ret = malloc( sizeof( cnrbtree_##key_t##data_t ) ); \
		memset( ret, 0, sizeof( *ret ) ); \
		return ret; \
	}\
	functiondecorator cnrbtree_##key_t##data_t##_node * cnrbtree##key_t##data_t##getltgt( cnrbtree_##key_t##data_t * tree, key_t doptrkey key, int lt, int gt ) \
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

