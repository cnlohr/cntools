#include <stdint.h>
#include <stdio.h>
#include <math.h>


int main()
{
	int N_WAVE = 4096;

	printf( "int32_t Sinewave[N_WAVE-N_WAVE/4] = {\n" );
	int i;
	for( i = 0; i < N_WAVE-N_WAVE/4; i++ )
	{
		printf( "%11d, ", (int)(sin( i * 3.1415926 * 2 / N_WAVE ) * ((1ULL<<31)-1)) );
		if( ( i % 6 ) == 5 )
		{
			printf( "\n" );
		}
	}
		
	return 0;
}

