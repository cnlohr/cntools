
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

using namespace std;


int main()
{
	map< string, string > tmap;

	srand(0);
	#define ITERATIONS 1000
	int i, j;
	for( j = 0; j < 3000; j++ )
	{
		char stta[ITERATIONS][9];
		for( i = 0; i < ITERATIONS; i++ )
		{
			int k;
			for( k = 0; k < 8; k++ ) 
				stta[i][k] = (rand()%26) + 'a';
			stta[i][k] = 0;

			if( tmap.find( stta[i] ) != tmap.end() )
			{
				printf( "Duplicate.  Try again.\n" );
				exit( 5 );
			}

			tmap[stta[i]] = stta[i];
		}


		for( i = 0; i < ITERATIONS; i++ )
		{
			map< string, string >::iterator it = tmap.find( stta[i] );
			if( it == tmap.end() )
			{
				printf( "Access fault.\n" );
				exit( 5 );
			}
			tmap.erase( it );
		}

	}


}
