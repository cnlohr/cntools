
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

using namespace std;

class QuickComp
{
public:
	QuickComp( string s ) { str = s; }
	int compare( const QuickComp & rhs) const { int64_t r = *(int64_t*)str.c_str() - *(int64_t*)rhs.str.c_str(); return (r == 0) ? 0 :( ( r < 0 )? -1 : 1 ); }
	const bool operator < ( const QuickComp & rhs ) const { return compare( rhs ) < 0 ; }
	const bool operator == ( const QuickComp & rhs ) const { return compare( rhs ) == 0 ; }
	string str;
};

int main()
{
	map< QuickComp, string > tmap;
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

			if( tmap.find( QuickComp(stta[i]) ) != tmap.end() )
			{
				printf( "Duplicate.  Try again.\n" );
				exit( 5 );
			}

			tmap[QuickComp(stta[i])] = stta[i];
		}


		for( i = 0; i < ITERATIONS; i++ )
		{
			map< QuickComp, string >::iterator it = tmap.find( QuickComp(stta[i]) );
			if( it == tmap.end() )
			{
				printf( "Access fault.\n" );
				exit( 5 );
			}
			tmap.erase( it );
		}

	}


}
