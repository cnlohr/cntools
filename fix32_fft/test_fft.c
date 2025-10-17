#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define FIX32_FFT_IMPLEMENTATION
#define FIX32_FFT_PRECISEROUNDING 1

#include "fix32_fft.h"

#define M 14

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
			(rand()%2000000)-1000000;
		imag[i] = 
			//cos( i ) * ((1LL<<8)-1);
			(rand()%2000000)-1000000;
	}

	int offset = 100;
	int nmatch = 2048;

	// The limit is about... At least I think that's how it works?
	// Same equation as oversampling, right?
	//
	//    -14dB (/5 in voltage) for 128 samples.
	//    -20dB (/10 in voltage) for 512 samples.
	//    -26dB (/20 in voltage) for 2048 samples.
	//    -32dB (/40 in voltage) for 8192 samples (though 16384 seems to be preferred)

	for( i = 0; i < nmatch; i++ )
	{
		int rv = convmapreal[i] = (rand()%2000000)-1000000;
		int iv = convmapimag[i] = (rand()%2000000)-1000000;

		// Mix new random signal in.
		real[offset+i] += rv/20;
		imag[offset+i] += iv/20;
	}

	fCheck = fopen ("test_convC.csv","w");

	for( i = 0; i < (1<<M); i++ )
	{
		// i is the offset.
		double ar = 0;
		double ai = 0;
		for( int j = 0; j < nmatch; j++ )
		{
			if( j+i > (1<<M) ) continue;
			ar += real[j+i] * (double)convmapreal[j] + imag[j+i] * (double)convmapimag[j];
			ai +=-imag[j+i] * (double)convmapreal[j] + real[j+i] * (double)convmapimag[j];
		}
		fprintf( fCheck, "%d, %f, %f, %f\n", i, ar/8192.0/8192.0, ai/8192.0/8192.0, sqrt(ar*ar+ai*ai)/8192.0/8192.0 );
		// You can make sure the convolution works by checking these two against each other.
	}

	fclose( fCheck );

	if( fix32_fft( real, imag, M, 0 ) ) goto fail;
	if( fix32_fft( convmapreal, convmapimag, M, 0 ) ) goto fail;

	fCheck = fopen ("test_intermediate.csv","w");

	for( i = 0; i < (1<<M); i++ )
	{
		int32_t ri = real[i];
		int32_t ii = imag[i];
		int32_t cmrr = convmapreal[i];
		int32_t cmri = convmapimag[i];

		// Performing a conjugate multiply, so it's a correlation not a convolution.
		int32_t t = ((ri * (int64_t)cmrr)>>24) + ((ii * (int64_t)cmri)>>24);
		imag[i]   =-((ri * (int64_t)cmri)>>24) + ((ii * (int64_t)cmrr)>>24);
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
		double amp = sqrt(real[i] * (double)real[i] + imag[i] * (double)imag[i]);
		fprintf( fCheck, "%d, %d, %d, %f\n", i, real[i], imag[i], sqrt(real[i]*(double)real[i] + imag[i]*(double)imag[i]) );
		pwrRMS += (amp);
		//pwrRMS +=(amp);
		if( i == offset ) pwrPeak = amp;
	}
	pwrRMS = pwrRMS/(1<<M);
	printf( "RMS: %f  Expected Peak: %f / Ratio: %f\n", pwrRMS, pwrPeak, pwrPeak/pwrRMS );
	fclose( fCheck );

	return 0;
fail:
	fprintf( stderr, "Error running 32-bit FFT.\n" );
	return -1;
}

