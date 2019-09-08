//Public Domain 2019 <>< Charles Lohr
#ifndef _CNHASH_H
#define _CNHASH_H

#include <stdint.h>

//This hast table is *not* threadsafe!

typedef struct cnhashelement_t
{
	void *   key;
	void *   data;
	intptr_t hashvalue;	//MUST be nonzero if populated.
} cnhashelement;

typedef void *   (*cnhash_copy_key_function)( const void * keyin );
typedef intptr_t (*cnhash_hash_function)( const void * key, void * opaque );
typedef int      (*cnhash_compare_function)( const void * key_a, const void * key_b, void * opaque );
typedef void     (*cnhash_delete_function)( void * key, void * data, void * opaque );

typedef struct cnhashtable_t
{
	int nr_elements;
	int array_size;
	cnhashelement * elements;
	void * opaque;
	cnhash_copy_key_function kf;
	cnhash_hash_function hf;
	cnhash_compare_function cf;
	cnhash_delete_function df;
	uint8_t table_size_index;
	uint8_t allow_duplicates;
} cnhashtable;

cnhashtable * CNHashGenerate( int allow_duplicates, void * opaque, cnhash_copy_key_function kf, cnhash_hash_function hf, cnhash_compare_function cf, cnhash_delete_function df );
cnhashelement * CNHashInsert( cnhashtable * ht, const void * key, void * data ); //return 0 if fail... If duplicates not allowed, and existent element, returns ptr to that element.  Does not overwrite data on collision.
cnhashelement * CNHashGet( cnhashtable * ht, const void * key );
cnhashelement * CNHashGetMultiple( cnhashtable * ht, const void * key, int * nrvalues );
cnhashelement * CNHashIndex( cnhashtable * ht, const void * key ); //If get fails, insert... Same as Insert for non-duplicate hashes.
void * CNHashGetValue( cnhashtable * ht, const void * key );
void ** CNHashGetValueMultiple( cnhashtable * ht, const void * key, int * nrvalues );
void CNHashDelete( cnhashtable * ht, const void * key );
void CNHashDestroy( cnhashtable * ht );

#ifndef HASHNODEBUG
void CNHashDump( cnhashtable * ht );
#endif

//String-version assumes both key and data are malloc'd ptrs that will be freed.
void *   cnhash_strkf( const void * keyin );
intptr_t cnhash_strhf( const void * key, void * opaque );
int      cnhash_strcf( const void * key_a, const void * key_b, void * opaque );
void     cnhash_strdel( void * key, void * data, void * opaque );
#define CNHASH_STRINGS cnhash_strkf, cnhash_strhf, cnhash_strcf, cnhash_strdel

intptr_t cnhash_ptrhf( const void * key, void * opaque );
int      cnhash_ptrcf( const void * key_a, const void * key_b, void * opaque );
#define CNHASH_POINTERS 0, cnhash_ptrhf, cnhash_ptrcf, 0

#endif

