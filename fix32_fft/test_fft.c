#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define FIX32_FFT_IMPLEMENTATION
#define FIX32_FFT_PRECISEROUNDING 1

#include "fix32_fft.h"

#define M 12

uint64_t deviation( int a, int b )
{
	return (a-b)*(a-b);
}

int32_t real[1<<M] = { 0 };
int32_t imag[1<<M] = { 0 };
int32_t convmapreal[1<<M] = { 0 };
int32_t convmapimag[1<<M] = { 0 };

int main()
{
	// Steps are:
	// 1. Perform a forward and reverse complex FFT.
	// 2. Perform a forward and reverse real FFT.
	// 3. Perform a correlation (the mirror of a convolution)

	int i;

	srand(1);
	for( i = 0; i < (1<<M); i++ )
	{
		real[i] = 
//			sin( i ) * ((1LL<<10)-1);
			(rand()%10001)-5000;
		imag[i] = 
//			cos( i ) * ((1LL<<10)-1);
			(rand()%10001)-5000;
	}

	if( fix32_fft( real, imag, M, 0 ) ) goto fail;
//	fix32_shift( real, imag, M, -M/2 );
	if( fix32_fft( real, imag, M, 1 ) ) goto fail;
//	fix32_shift( real, imag, M, -M/2 );

	srand(1);
	FILE * fCheck = fopen( "test_fwdrev.csv", "w" );

	uint64_t total_deviation = 0;
	for( i = 0; i < (1<<M); i++ )
	{
		int rcheck = 
			// sin( i ) * ((1LL<<10)-1);
			(rand()%10001)-5000;
		int icheck =
			//cos( i ) * ((1LL<<10)-1);
			(rand()%10001)-5000;

		fprintf( fCheck, "%d, %d, %d, %d\n", rcheck, icheck, real[i], imag[i] );
		//if( i < 50 )
		//	printf( "%d  %d\n", rcheck, real[i] );
		total_deviation +=
			deviation( real[i], rcheck ) +
			deviation( imag[i], icheck );
	}
	fclose( fCheck );

	printf( "Complex FFT RMS Error Squared Per Sample: %f\n", total_deviation/(double)(1<<M) );

	srand(1);

	for( i = 0; i < (1<<M); i++ )
	{
		real[i] = sin( i * 0.2 ) * ((1LL<<10)-1); //(rand()%10001)-5000;
	}

	if( fix32_fftr( real, M, 0 ) ) goto fail;
	if( fix32_fftr( real, M, 1 ) ) goto fail;
	fix32_shift( real, imag, M, -1 );

	srand(1);

	fCheck = fopen( "test_real.csv", "w" );

	total_deviation = 0;
	for( i = 0; i < (1<<M); i++ )
	{
		int rcheck = sin( i * 0.2 ) * ((1LL<<10)-1); //(rand()%10001)-5000;

		fprintf( fCheck, "%d, %d\n", real[i], rcheck );
		//if( i < 50 )
		//printf( "%d  %d\n", rcheck, real[i] );
		total_deviation += deviation( real[i], rcheck );
	}

	printf( "Real FFT RMS Error Squared Per Sample: %f\n", total_deviation/(double)(1<<M) );
	fclose( fCheck );

	srand(1);

	for( i = 0; i < (1<<M); i++ )
	{
		real[i] = 
			sin( i ) * ((1LL<<8)-1);
			//(rand()%1001)-500;
		imag[i] = 
			cos( i ) * ((1LL<<8)-1);
			//(rand()%1001)-500;
	}

	srand(2);

	int offset = 500;

	for( i = 0; i < 512; i++ )
	{
		int rv = convmapreal[i] = (rand()%1001)-500;
		int iv = convmapimag[i] = (rand()%1001)-500;

		// Mix new random signal in.
		real[i+offset] += rv;
		imag[i+offset] += iv;
	}

	if( fix32_fft( real, imag, M, 0 ) ) goto fail;
	if( fix32_fft( convmapreal, convmapimag, M, 0 ) ) goto fail;

	fCheck = fopen ("test_intermediate.csv","w");

	for( i = 0; i < (1<<M); i++ )
	{
		fprintf( fCheck, "%d, %d\n", real[i], imag[i] );
		int32_t t = ((real[i] * convmapreal[i])>>31) + ((imag[i] * convmapimag[i])>>31);
		imag[i]   =-((real[i] * convmapimag[i])>>31) + ((imag[i] * convmapreal[i])>>31);
		real[i] = t;
	}

	fclose( fCheck );

	if( fix32_fft( real, imag, M, 1 ) ) goto fail;

	fCheck = fopen ("test_conv.csv","w");
	for( i = 0; i < (1<<M); i++ )
	{
		fprintf( fCheck, "%d, %d, %d, %d\n", i, real[i], imag[i], real[i]*real[i] + imag[i]*imag[i] );
	}
	fclose( fCheck );

	return 0;
fail:
	fprintf( stderr, "Error running 32-bit FFT.\n" );
	return -1;
}

