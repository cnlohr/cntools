//Public Domain 2019 <>< Charles Lohr

#include "cnhash.h"
#include <string.h>
#include <stdlib.h>

static const int cnhashtablesizes[] = {
	17, 43, 109, 313, 1013, 3049, 7057,
	18043, 41177, 104723, 300073, 842353,
	1654579, 3651673, 9445001, 19438681, 49408223, //17 total.
	100906297, 245673083, 559567819, 1245665153,
	2147483563 };
	
	

cnhashtable * CNHashGenerate( int allow_duplicates, void * opaque, cnhash_hash_function hf, cnhash_compare_function cf, cnhash_delete_function df )
{
	cnhashtable * ret = malloc( sizeof( cnhashtable ) );
	ret->nr_elements = 0;
	ret->table_size_index = 0;
	ret->array_size = cnhashtablesizes[ret->table_size_index];
	ret->allow_duplicates = allow_duplicates;
	int bytes = ret->array_size * sizeof( cnhashelement );
	ret->elements = malloc( bytes );
	memset( ret->elements, 0, bytes );
	ret->opaque = opaque;
	ret->hf = hf;
	ret->cf = cf;
	ret->df = df;
	return ret;
}

int CNHashInsert( cnhashtable * ht, void * key, void * data )
{
	if( ( ht->nr_elements + 1 ) > ( ht->array_size / 2 ) )
	{
		//HT too small.  Increase size.
		if( (ht->table_size_index + 1) >= sizeof( cnhashtablesizes ) / sizeof( cnhashtablesizes[0] ) )
			return -1;

		int old_array_size = ht->array_size;
		cnhashelement * old_array = ht->elements;
		int array_size = ht->array_size = cnhashtablesizes[++ht->table_size_index];
		int array_size_bytes = sizeof( cnhashelement ) * array_size;

		ht->elements = malloc( array_size_bytes );
		memset( ht->elements, 0, array_size_bytes );
		int i;
		for( i = 0; i < old_array_size; i++ )
		{
			if( old_array[i].hashvalue )
				CNHashInsert( ht, old_array[i].key, old_array[i].data );
		}
		free( old_array );
	}
	uint32_t hash = ht->hf( key, ht->opaque );
	int i = hash % ht->array_size;
	cnhashelement * he;
	if( ht->allow_duplicates )
	{
		while( ( he = &ht->elements[i] )->hashvalue )
		{
			i = (i+1) % ht->array_size;
		}
	}
	else
	{
		while( ( he = &ht->elements[i] )->hashvalue )
		{
			if( ht->elements[i].hashvalue == hash )
			{
				if( ht->cf( he->key, key, ht->opaque ) == 0 ) break;
			}
			i = (i+1) % ht->array_size;
		}
	}
	he->key = key;
	he->hashvalue = hash;
	he->data = data;

	ht->nr_elements++;
	return 0;
}

cnhashelement * CNHashGet( cnhashtable * ht, void * key )
{
	uint32_t hash = ht->hf( key, ht->opaque );
	int i = hash % ht->array_size;
	cnhashelement * he;
	do
	{
		he = &ht->elements[i];
		if( he->hashvalue == hash )
		{
			if( ht->cf( he->key, key, ht->opaque ) == 0 ) return he;
		}
		i = (i+1) % ht->array_size;
	} while( he->hashvalue );
	return 0;
}

cnhashelement * CNHashGetMultiple( cnhashtable * ht, void * key, int * nrvalues )
{
	*nrvalues = 0;
	cnhashelement * ret = 0;

	uint32_t hash = ht->hf( key, ht->opaque );
	int i = hash % ht->array_size;
	cnhashelement * he;
	do
	{
		he = &ht->elements[i];
		if( he->hashvalue == hash )
		{
			if( ht->cf( he->key, key, ht->opaque ) == 0 )
			{
				(*nrvalues)++;
				if( !ret )
					ret = malloc( sizeof(cnhashelement) );
				else
					ret = realloc( ret, sizeof(cnhashelement)*(*nrvalues) );

				cnhashelement * e = ret + ((*nrvalues)-1);
				memcpy( e, he, sizeof(*he) );				
				if( !ht->allow_duplicates )
				{
					return ret;
				}
			}
		}
		i = (i+1) % ht->array_size;
	} while( he->hashvalue );
	return ret;

}

void * CNHashGetValue( cnhashtable * ht, void * key )
{
	cnhashelement * e = CNHashGet( ht, key );
	if( e ) return e->data;
	else return 0;
}

void ** CNHashGetValueMultiple( cnhashtable * ht, void * key, int * nrvalues )
{
	cnhashelement * r = CNHashGetMultiple( ht, key, nrvalues );
	void ** ret = malloc( sizeof( void * ) * (*nrvalues) );
	int i, v = *nrvalues;
	for( i = 0; i < v; i++ )
	{
		ret[i] = r[i].data;
	}
	free( r );
	return ret;
}


void CNHashDelete( cnhashtable * ht, void * key )
{
	int nrvalues;
	cnhashelement * hes = CNHashGetMultiple( ht, key, &nrvalues );
	int i;
	uint32_t hash = ht->hf( key, ht->opaque );

	for( i = 0; i < ht->array_size; i++ )
	{
		cnhashelement * he = &ht->elements[i];
		if( he->hashvalue == hash )
		{
			ht->df( he->key, he->data, ht->opaque );
			he->key = 0;
			he->data = 0;
			he->hashvalue = 0;
		}
	}
}

void CNHashDestroy( cnhashtable * ht )
{
	int i;
	for( i = 0; i < ht->array_size; i++ )
	{
		cnhashelement * he = &ht->elements[i];
		if( he->hashvalue )
			ht->df( he->key, he->data, ht->opaque );
	}
	free( ht->elements );
	free( ht );
}


uint32_t cnhash_strhf( void * key, void * opaque )
{
	//SDBM (public domain) http://www.cse.yorku.ca/~oz/hash.html
	char * str = (char*)key;
	unsigned long hash = 0;
	int c;
	while ( (c = *str++ ) )
		hash = c + (hash << 6) + (hash << 16) - hash;
	if( !hash )
		return 1;
	return hash;
}

int      cnhash_strcf( void * key_a, void * key_b, void * opaque )
{
	return strcmp( key_a, key_b );
}

void     cnhash_strdel( void * key, void * data, void * opaque )
{
	if( key )  free( key );
	if( data ) free( data );
}





