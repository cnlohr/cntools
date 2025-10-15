// Complex rotation approach (From willmore)
// Spectacular for small N, worse for larger N.
// @ N = 8192, towards the end, typically off by ~2-3k LSB per event.
// This is actually a hybrid approach, using willmore rotations, but syncing up to the sinetable when appropraite.

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


#define SINESIZEL2 12
#define SINESIZE (1<<SINESIZEL2)
#define SINEMASK (SINESIZE-1)
#define LOG2_N_WAVE 24
#define N_WAVE (1<<LOG2_N_WAVE)


int precise = 0;

int32_t MUL_MPYU( int32_t a, uint32_t b )
{
	if( !precise )
	{
		return ((int64_t)a * b) >> 32;
	}
	else
	{
		int64_t rc = ((int64_t)a*b)>>31;
		int rca = rc & 1;
		return (rc>>1) + rca;
	}
}


/*

Originally I tried using precise rounding, but turns out it doesn't actually help for these phasors.

static inline int32_t MUL_MPYU( int32_t a, uint32_t b )
{
#ifdef FIX32_FFT_PRECISEROUNDING
	int64_t rc = ((int64_t)a*(uint64_t)b)>>31;
	int32_t rca = rc & 1;
	return (rc>>1) + rca;
#else
	return ((int64_t)a * (uint64_t)b) >> 32;
#endif
}

static inline int32_t MUL_MPYB( int32_t a, int32_t b )
{
#ifdef FIX32_FFT_PRECISEROUNDING
	int64_t rc = ((int64_t)a*(uint64_t)b)>>31;
	int32_t rca = rc & 1;
	return (rc>>1) + rca;
#else
	return ((int64_t)a * (uint64_t)b) >> 32;
#endif
}
//				int32_t wprimer =  MUL_MPYU( wr, br ) - MUL_MPYB( wi, bi );
//				wi =               MUL_MPYU( wi, br ) + MUL_MPYB( wr, bi );
*/

int main()
{
	int i;

	int adjx, adjy;

	FILE * fOut = fopen( "luts.txt", "w" );
	int32_t sinetable_base[SINESIZE + SINESIZE/4];
	fprintf( fOut, "#define SINESIZEL2 %d\n", SINESIZEL2 );
	fprintf( fOut, "#define SINESIZE (1<<SINESIZEL2)\n" );
	fprintf( fOut, "#define SINEMASK (SINESIZE-1)\n" );
	fprintf( fOut, "#define LOG2_N_WAVE %d\n", LOG2_N_WAVE );
	fprintf( fOut, "#define N_WAVE %d\n\n", N_WAVE );

	fprintf( fOut, "int32_t sinetable_base[SINESIZE + SINESIZE/4] = {\n" );
	for( i = 0; i < SINESIZE + SINESIZE/4; i++ )
	{
		if( (i % 6) == 0 ) fprintf( fOut, "\t" );
		int32_t v = sinetable_base[i] = (int32_t)(sin( i * 3.1415926535 * 2 / SINESIZE ) * ((1ULL<<31)-1)+0.5);
		fprintf( fOut, "%11d,", v);
		if( (i % 6) == 5 ) fprintf( fOut, "\n" );
	}
	fprintf( fOut, "};\n\n" );

	// After a lot of experimentation I could not see any actual
	// difference in the quality of the output FFT validation
	// if you use precise phasor computations.

	//for( precise = 0; precise < 2; precise++ )
	precise = 0;
	{
//		if( precise == 0 )
//			fprintf( fOut, "#ifndef FIX32_FFT_PRECISEROUNDING\n\n" );
//		else
//			fprintf( fOut, "#else\n\n" );
		fprintf( fOut, "uint32_t rotationsets[(LOG2_N_WAVE-SINESIZEL2)*2] = {\n" );

		int suba;
		for( suba = 0; suba <= LOG2_N_WAVE; suba++ )
		{
			int rstop = (1<<(suba));

			int32_t bestx, besty;
			uint32_t bestbr, bestbi;
			double minerror = 1e30;

			//for( adjsx = -10; adjsx <= 5; adjsx++ )
			//for( adjsy = -10; adjsy <= 5; adjsy++ )
			for( adjx = -4; adjx <= 3; adjx++ )
			for( adjy = -4; adjy <= 3; adjy++ )
			{
				int usex = adjx, usey = adjy;
				if( adjx == -4 ) usex = 0;
				if( adjx == 0 ) continue;
				if( adjy == -4 ) usey = 0;
				if( adjy == 0 ) continue;

				double deltaomega =  3.141592653589*2.0/rstop;

				int32_t ar = 0;
				int32_t ai = (1ULL<<31)-1;

				// Tricky: We can use uint32_t's here.
				uint32_t br = cos( deltaomega ) * ((1ULL<<32)-1) + adjx;	
				uint32_t bi = (sin( deltaomega ) * ((1ULL<<32)-1)) + adjy;

				double omega = 0;
				double error = 0;

				int sinelookuploc = 0;
				int sinecheck = 1;

				for( i = 0; i < rstop; i++ )
				{
					int64_t targetar = sin( omega ) * ((1ULL<<31)-1);
					int64_t targetai = cos( omega ) * ((1ULL<<31)-1);

					if( --sinecheck == 0 )
					{
						ar = sinetable_base[sinelookuploc];
						ai = sinetable_base[sinelookuploc+SINESIZE/4];
						if( suba >= SINESIZEL2 )
						{
							sinelookuploc++;
							sinecheck = 1<<(suba-SINESIZEL2);
						}
						else
						{
							sinelookuploc += 1<<(SINESIZEL2 - suba);
							sinecheck = 1;
						}
					}
					else
					{
						// a = a * b where b is the amount we want to rotate.
						// We are doing a conjugate multiply here, so we can store bi as a positive value instead
						// of a negative value.
						// These multiplies are mulhsu
						int32_t aprimer =  (MUL_MPYU(ar, br)) + (MUL_MPYU(ai, bi));
						ai =               (MUL_MPYU(ai, br)) - (MUL_MPYU(ar, bi));
						ar = aprimer;
					}

					error+=(((double)ar-targetar)*((double)ar-targetar) + ((double)ai-targetai)*((double)ai-targetai));

					omega += deltaomega;
				}

				if( error < minerror )
				{
					minerror = error;
					bestx = usex;
					besty = usey;
					bestbr = br;
					bestbi = bi;
				}
			}
			printf( "Suba: %2d ", suba );
			printf( "Best: %3d %3d (%f err per samp)\n", bestx, besty, sqrt(minerror/rstop) );
			if( suba > SINESIZEL2 )
				fprintf( fOut, "\t%11u, %11u,\n", bestbr, bestbi );
		}
		fprintf( fOut, "};\n\n" );
	}
//	fprintf( fOut, "#endif\n" );
	fclose( fOut );
}


