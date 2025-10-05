#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define FIX32_FFT_IMPLEMENTATION
//#define FIX32_FFT_PRECISEROUNDING

#include "fix32_fft.h"

#define M 10

uint64_t deviation( int a, int b )
{
	int diff = a - b;
	if( diff < 0 )
		return -diff;
	else
		return diff;
}

int main()
{
	int32_t real[1<<M] = { 0 };
	int32_t imag[1<<M] = { 0 };

	int i;

	srand(1);
	for( i = 0; i < 1024; i++ )
	{
		real[i] = sin( i ) * 16380000; //(rand()%10001)-5000;
		imag[i] = cos( i ) * 16380000; //(rand()%10001)-5000;
	}
	
	if( fix_fft( real, imag, M, 0 ) ) goto fail;

	if( fix_fft( real, imag, M, 1 ) ) goto fail;

	srand(1);

	uint64_t total_deviation = 0;
	for( i = 0; i < 1024; i++ )
	{
		int rcheck = sin( i ) * 16380000; //(rand()%10001)-5000;
		int icheck = cos( i ) * 16380000; //(rand()%10001)-5000;
//		printf( "%d  %d\n", rcheck, real[i] );
		total_deviation +=
			deviation( real[i], rcheck ) +
			deviation( imag[i], icheck );
	}

	printf( "Deviation: %lu\n", total_deviation );

	return 0;
fail:
	fprintf( stderr, "Error running 32-bit FFT.\n" );
	return -1;
}

