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
			//sin( i ) * ((1LL<<8)-1);
			(rand()%10000000)-5000000;
		imag[i] = 
			//cos( i ) * ((1LL<<8)-1);
			(rand()%10000000)-5000000;
	}

	int offset = 800;

	for( i = 0; i < 1024; i++ )
	{
		int rv = convmapreal[i] = (rand()%10000000)-5000000;
		int iv = convmapimag[i] = (rand()%10000000)-5000000;

		// Mix new random signal in.
		real[offset+i] += rv/8;
		imag[offset+i] += iv/8;
	}

	if( fix32_fft( real, imag, M, 0 ) ) goto fail;
	if( fix32_fft( convmapreal, convmapimag, M, 0 ) ) goto fail;

	fCheck = fopen ("test_intermediate.csv","w");

	for( i = 0; i < (1<<M); i++ )
	{
		int32_t ri = real[i];
		int32_t ii = imag[i];
		int32_t cmrr = convmapreal[i];
		int32_t cmri = convmapimag[i];
		int32_t t = ((ri * (int64_t)cmrr)>>36) + ((ii * (int64_t)cmri)>>36);
		imag[i]   =-((ri * (int64_t)cmri)>>36) + ((ii * (int64_t)cmrr)>>36);
		real[i] = t;
		fprintf( fCheck, "%d, %d, %d, %d, %d, %d\n", ri,ii, cmrr, cmri, real[i], imag[i] );
	}

	fclose( fCheck );

	if( fix32_fft( real, imag, M, 1 ) ) goto fail;

	fCheck = fopen ("test_conv.csv","w");
	double pwrRMS = 0;
	double pwrPeak = 0;
	for( i = 0; i < (1<<M); i++ )
	{
		double amp = real[i] * real[i] + imag[i] * imag[i];
		fprintf( fCheck, "%d, %d, %d, %d\n", i, real[i], imag[i], real[i]*real[i] + imag[i]*imag[i] );
		pwrRMS += (amp);
		//pwrRMS +=(amp);
		if( i == 800 ) pwrPeak = amp;
	}
	pwrRMS = pwrRMS/(1<<M);
	printf( "%f %f\n", pwrRMS, pwrPeak );
	fclose( fCheck );

	return 0;
fail:
	fprintf( stderr, "Error running 32-bit FFT.\n" );
	return -1;
}

