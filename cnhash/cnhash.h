//Public Domain 2019 <>< Charles Lohr
#ifndef _CNHASH_H
#define _CNHASH_H

#include <stdint.h>

//This hast table is *not* threadsafe!

typedef struct cnhashelement_t
{
	void *   key;
	uint32_t hashvalue;	//MUST be nonzero.
	void * data;
} cnhashelement;

typedef uint32_t (*cnhash_hash_function)( void * key, void * opaque );
typedef int      (*cnhash_compare_function)( void * key_a, void * key_b, void * opaque );
typedef void     (*cnhash_delete_function)( void * key, void * data, void * opaque );

typedef struct cnhashtable_t
{
	int nr_elements;
	int array_size;
	cnhashelement * elements;
	void * opaque;
	cnhash_hash_function hf;
	cnhash_compare_function cf;
	cnhash_delete_function df;
	uint8_t table_size_index;
	uint8_t allow_duplicates;
} cnhashtable;

cnhashtable * CNHashGenerate( int allow_duplicates, void * opaque, cnhash_hash_function hf, cnhash_compare_function cf, cnhash_delete_function df );
int CNHashInsert( cnhashtable * ht, void * key, void * data ); //return 0 if successful.
cnhashelement * CNHashGet( cnhashtable * ht, void * key );
cnhashelement * CNHashGetMultiple( cnhashtable * ht, void * key, int * nrvalues );
cnhashelement * CNHashIndex( cnhashtable * ht, void * key ); //If get fails, insert
void * CNHashGetValue( cnhashtable * ht, void * key );
void ** CNHashGetValueMultiple( cnhashtable * ht, void * key, int * nrvalues );
void CNHashDelete( cnhashtable * ht, void * key );
void CNHashDestroy( cnhashtable * ht );

#ifndef HASHNODEBUG
void CNHashDump( cnhashtable * ht );
#endif

//String-version assumes both key and data are malloc'd ptrs that will be freed.
uint32_t cnhash_strhf( void * key, void * opaque );
int      cnhash_strcf( void * key_a, void * key_b, void * opaque );
void     cnhash_strdel( void * key, void * data, void * opaque );
#define CNHASH_STRINGS cnhash_strhf, cnhash_strcf, cnhash_strdel

#endif

