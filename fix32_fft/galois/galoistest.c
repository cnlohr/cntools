#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define FIX32_FFT_IMPLEMENTATION
#define FIX32_FFT_PRECISEROUNDING 1

#include "fix32_fft.h"

#define M 8

#define MM ((1<<M))

int32_t real[1<<M] = { 0 };
int32_t imag[1<<M] = { 0 };
int32_t convmapreal[1<<M] = { 0 };
int32_t convmapimag[1<<M] = { 0 };

int32_t rscram[1<<M] = { 0 };
int32_t iscram[1<<M] = { 0 };


/*
int scramble_fwd( int l )
{
	// Hacker's delight for bit reversal, "rev1" - basic bit reversal
	uint32_t tmp = (l & 0x55555555) << 1 | (l & 0xAAAAAAAA) >>  1;
	tmp = (tmp & 0x33333333) <<  2 | (tmp & 0xCCCCCCCC) >>  2;
	tmp = (tmp & 0x0F0F0F0F) <<  4 | (tmp & 0xF0F0F0F0) >>  4;
	tmp = (tmp & 0x00FF00FF) <<  8 | (tmp & 0xFF00FF00) >>  8;
	tmp = (tmp & 0x0000FFFF) << 16 | (tmp & 0xFFFF0000) >> 16;
	uint32_t mr = tmp >> (32-M);
	return mr;
}
*/


// from https://gist.github.com/chris-gc/8445626

#if 0
static const uint8_t INV_TABLE[256] = {
        0, 1, 175, 202, 248, 70, 101, 114, 124, 46, 35, 77, 157, 54, 57, 247,
        62, 152, 23, 136, 190, 244, 137, 18, 225, 147, 27, 26, 179, 59, 212, 32,
        31, 213, 76, 10, 164, 182, 68, 220, 95, 144, 122, 113, 235, 195, 9, 125,
        223, 253, 230, 189, 162, 120, 13, 156, 246, 14, 178, 29, 106, 84, 16, 153,
        160, 119, 197, 198, 38, 221, 5, 249, 82, 159, 91, 207, 34, 11, 110, 166,
        128, 104, 72, 158, 61, 107, 151, 201, 218, 116, 206, 74, 171, 155, 145, 40,
        192, 139, 209, 134, 115, 6, 241, 180, 81, 129, 60, 85, 169, 176, 78, 167,
        123, 43, 7, 100, 89, 219, 161, 65, 53, 163, 42, 112, 8, 47, 227, 187,
        80, 105, 148, 232, 205, 214, 99, 208, 19, 22, 193, 97, 173, 229, 211, 238,
        41, 94, 224, 25, 130, 233, 200, 86, 17, 63, 170, 93, 55, 12, 83, 73,
        64, 118, 52, 121, 36, 183, 79, 111, 177, 108, 154, 92, 228, 140, 203, 2,
        109, 168, 58, 28, 103, 240, 37, 165, 250, 217, 226, 127, 231, 51, 20, 245,
        96, 138, 234, 45, 199, 66, 67, 196, 150, 87, 3, 174, 215, 132, 90, 75,
        135, 98, 239, 142, 30, 33, 133, 204, 251, 185, 88, 117, 39, 69, 252, 48,
        146, 24, 186, 126, 172, 141, 50, 188, 131, 149, 194, 44, 255, 243, 143, 210,
        181, 102, 254, 237, 21, 191, 56, 15, 4, 71, 184, 216, 222, 49, 242, 236
};
#endif

static uint8_t GFtable[256];


int scramble_fwd( int l )
{
	//return INV_TABLE[l];
	return GFtable[l];
}

int main()
{
	int i;
	int b = 1;
	for( i = 0; i < 256; i++ )
	{
		// Use GF(2^8) polynomial to create 
	    if (b & (1<<8)) b = (b ^ 0435) & 0xff;

		GFtable[b] = i;
		b = b<<1;
	}

//	for( i = 0; i < 256; i++ )
//	{
//		printf( "%d %d\n", i, GFtable[i] );
//s	}

	FILE * fCheck;
	fCheck = fopen ("test_conv.csv","w");

	for( int skew = 0; skew < 256; skew++ )
	{

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

		// The limit is about... At least I think that's how it works?
		// Same equation as oversampling, right?
		//
		//    -14dB (/5 in voltage) for 128 samples.
		//    -20dB (/10 in voltage) for 512 samples.
		//    -26dB (/20 in voltage) for 2048 samples.
		//    -32dB (/40 in voltage) for 8192 samples (though 16384 seems to be preferred)

		int offset = 34;

		for( i = 0; i < (1<<M); i++ )
		{
			int j = scramble_fwd( ( i )%(1<<M) );

			int rv = convmapreal[(j+ offset )%(1<<M) ] = ((rand()%2)*2-1)*1000000;
			int iv = convmapimag[(j+ offset )%(1<<M) ] = ((rand()%2)*2-1)*1000000;

			// Mix new random signal in.
			real[(i+skew)%(1<<M)] += rv/10;
			imag[(i+skew)%(1<<M)] += iv/10;
		}


		for( i = 0; i < (1<<M); i++ )
		{
			int j = scramble_fwd( ( i )%(1<<M) );
			rscram[j] = real[i];
			iscram[j] = imag[i];
		}
#if 0
		for ( i = 1; i < (1<<M); ++i )
		{
			int mr = scramble_fwd( i );

			// Make sure when we swap, we don't swap them back.
			if (mr <= i) continue;

			FIX32FFT_SWAP( real[i], real[mr] );
			FIX32FFT_SWAP( imag[i], imag[mr] );
		}
#endif


		if( fix32_fft( rscram, iscram, M, 0 ) ) goto fail;

		if( fix32_fft( convmapreal, convmapimag, M, 0 ) ) goto fail;

		//fCheck = fopen ("test_intermediate.csv","w");

		for( i = 0; i < (1<<M); i++ )
		{
			int32_t ri = rscram[i];
			int32_t ii = iscram[i];
			int32_t cmrr = convmapreal[i];
			int32_t cmri = convmapimag[i];

			// Performing a conjugate multiply, so it's a correlation not a convolution.
			int32_t t = ((ri * (int64_t)cmrr)>>24) + ((ii * (int64_t)cmri)>>24);
			iscram[i] =-((ri * (int64_t)cmri)>>24) + ((ii * (int64_t)cmrr)>>24);
			rscram[i] = t;
		//	fprintf( fCheck, "%d, %d, %d, %d, %d, %d\n", ri,ii, cmrr, cmri, real[i], imag[i] );
		}


		if( fix32_fft( rscram, iscram, M, 1 ) ) goto fail;

		double pwrRMS = 0;
		double pwrPeak = 0;
		for( i = 0; i < MM; i++ )
		{
			int j = i;//scramble_fwd( i );
			double amp = sqrt(rscram[j] * (double)rscram[j] + iscram[j] * (double)iscram[j]);
			fprintf( fCheck, "%9.1f, ", amp );

			pwrRMS += (amp);
			//pwrRMS +=(amp);
			if( i == offset ) pwrPeak = amp;
		}
		pwrRMS = pwrRMS/(1<<M);
		printf( "RMS: %f  Expected Peak: %f / Ratio: %f\n", pwrRMS, pwrPeak, pwrPeak/pwrRMS );
		fprintf( fCheck, "\n" );
	}
	fclose( fCheck );

	return 0;
fail:
	fprintf( stderr, "Error running 32-bit FFT.\n" );
	return -1;
}

