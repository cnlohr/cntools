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


#define SCRAMBLETABLE_BITS M

// from https://gist.github.com/chris-gc/8445626


// GF(2^8) polynomial creation, commonly used in Reed-Solomon coding.
// Both 0x11d and 0x187 are common for 8.
// I have no idea how they computed this table, but it works really well.
// https://github.com/nietoperz809/GF256/blob/master/galois.c
static int prim_poly[33] = 
{ 0, 
/*  1 */     1, 
/*  2 */    07,
/*  3 */    013,
/*  4 */    023,
/*  5 */    045,
/*  6 */    0103,
/*  7 */    0211,
/*  8 */    0435,
/*  9 */    01021,
/* 10 */    02011,
/* 11 */    04005,
/* 12 */    010123,
/* 13 */    020033,
/* 14 */    042103,
/* 15 */    0100003,
/* 16 */    0210013,
/* 17 */    0400011,
/* 18 */    01000201,
/* 19 */    02000047,
/* 20 */    04000011,
/* 21 */    010000005,
/* 22 */    020000003,
/* 23 */    040000041,
/* 24 */    0100000207,
/* 25 */    0200000011,
/* 26 */    0400000107,
/* 27 */    01000000047,
/* 28 */    02000000011,
/* 29 */    04000000005,
/* 30 */    010040000007,
/* 31 */    020000000011, 
/* 32 */    00020000007 };  /* Really 40020000007, but we're omitting the high order bit */


static uint16_t GFtable[1<<SCRAMBLETABLE_BITS];

void GenerateScrambleTable()
{
	int bits;
	int i;

	// Math validated using this calculator:
	// https://codyplanteen.com/assets/rs/gf256_log_antilog.pdf

	const uint32_t polynomial = prim_poly[SCRAMBLETABLE_BITS];

	int b = 1;
	for( i = 0; i < (1<<SCRAMBLETABLE_BITS); i++ )
	{
		//printf( "%d == %d\n", b, i );
		GFtable[b] = i;

		b = b<<1;
	    if (b & (1<<SCRAMBLETABLE_BITS))
			b = (b ^ polynomial);
		b = b & ((1<<SCRAMBLETABLE_BITS)-1);
	}
	int nrz = 0;
	for( i = 0; i < (1<<SCRAMBLETABLE_BITS); i++ )
	{
		if( GFtable[i] == 0 ) nrz++;
	}
	if( nrz != 1 )
	{
		fprintf( stderr, "Error: Polynomial table generation invalid\n" );
		exit( -1 );
	}
}

int scramble_fwd( int l )
{
	//return INV_TABLE[l];
	return GFtable[l];
}

int main()
{
	int i;
	GenerateScrambleTable();

	FILE * fCheck;
	fCheck = fopen ("test_conv.csv","w");

	for( int skew = 0; skew < MM; skew++ )
//	int skew = 0;
	{
		srand(skew);

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
		//    -32dB (/40 in voltage) for 8192 samples.

		double snr = 1.0/10.0;

		int offset = 34;

		for( i = 0; i < (1<<M); i++ )
		{
			int j = scramble_fwd( ( i )%(1<<M) );

			int rv = convmapreal[(j+ offset )%(1<<M) ] = ((rand()%2)*2-1)*1000000;
			int iv = convmapimag[(j+ offset )%(1<<M) ] = ((rand()%2)*2-1)*1000000;

			// Mix new random signal in.
			real[(i+skew)%(1<<M)] += rv*snr;
			imag[(i+skew)%(1<<M)] += iv*snr;
		}


		for( i = 0; i < (1<<M); i++ )
		{
			int j = scramble_fwd( ( i )%(1<<M) );
			rscram[j] = real[i];
			iscram[j] = imag[i];
		}


		if( fix32_fft( rscram, iscram, M, 0 ) ) goto fail;

		if( fix32_fft( convmapreal, convmapimag, M, 0 ) ) goto fail;

		//fCheck = fopen ("test_intermediate.csv","w");

		for( i = 0; i < (1<<M); i++ )
		{
			int32_t cmrr = rscram[i];
			int32_t cmri = iscram[i];
			int32_t carr = convmapreal[i];
			int32_t cari = convmapimag[i];

			// Performing a conjugate multiply, so it's a correlation not a convolution.
			int32_t t = ((carr * (int64_t)cmrr)>>25) + ((cari * (int64_t)cmri)>>25);
			iscram[i] =-((carr * (int64_t)cmri)>>25) + ((cari * (int64_t)cmrr)>>25);
			rscram[i] = t;
		//	fprintf( fCheck, "%d, %d, %d, %d, %d, %d\n", ri,ii, cmrr, cmri, real[i], imag[i] );
		}


		if( fix32_fft( rscram, iscram, M, 1 ) ) goto fail;

		double pwrRMS = 0;
		double pwrPeak = 0;
		fprintf( fCheck,"Skew %d,", skew );
		for( i = 0; i < MM; i++ )
		{
			int j = i;//scramble_fwd( i );
			double amp = sqrt(rscram[j] * (double)rscram[j] + iscram[j] * (double)iscram[j]);
			fprintf( fCheck, "%f,", amp );

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

