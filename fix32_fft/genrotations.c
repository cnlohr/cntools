// Complex rotation approach (From willmore)
// Spectacular for small N, worse for larger N.
// @ N = 8192, towards the end, typically off by ~2-3k LSB per event.

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main()
{
	int i;

	int adjx, adjy;

	double minerror = 1e30;
	int32_t bestx, besty;
	int32_t bestxa, bestya;

	int rstop = 8192;

	int adjsx, adjsy, bestadjsx, bestadjsy, bestadjsxa, bestadjsya;

	for( adjsx = -10; adjsx <= 5; adjsx++ )
	for( adjsy = -10; adjsy <= 5; adjsy++ )
	for( adjx = -5; adjx <= 5; adjx++ )
	for( adjy = -5; adjy <= 5; adjy++ )
	{
		double deltaomega =  3.141592653589*2.0/rstop;

		int32_t ar = 0 + adjsx;
		int32_t ai = (1ULL<<31)-1 + adjsy;
		int32_t sar = ar;
		int32_t sai = ai;
		int32_t br = cos( deltaomega ) * ((1ULL<<31)-1) + adjx;	
		int32_t bi =-sin( deltaomega ) * ((1ULL<<31)-1) + adjy;

		double omega = 0;
		double error = 0;
		// Similar to // https://amycoders.org/tutorials/sintables.html  (The Harmonical Oscilator Approach)
		for( i = 0; i <= rstop; i++ )
		{
			int32_t targetar = sin( omega ) * ((1ULL<<31)-1);
			int32_t targetai = cos( omega ) * ((1ULL<<31)-1);

			error+=((double)ar-targetar)*((double)ar-targetar) + ((double)ai-targetai)*((double)ai-targetai);
			if( adjx == 1 && adjy == -1 && adjsx == -9 && adjsy == 0 )
			{
				printf( "%d %d  Error:[%d %d]\n", ar, ai, ar-targetar, ai-targetai );
			}

			int32_t aprimer = ((ar * (int64_t)br)>>32) - ((ai * (int64_t)bi)>>32);
			ai = ((ar * (int64_t)bi)>>32) + ((ai * (int64_t)br)>>32);
			ar = aprimer<<1;
			ai = ai<<1;
			omega += deltaomega;
		}
		//printf( "Error: %f [%d %d] [%d %d]\n", sqrt(error/rstop), adjx, adjy, adjsx, adjsy );
		if( error < minerror )
		{
			printf( "New Best\n" );
			minerror = error;
			bestx = br;
			besty = bi;
			bestxa = adjx;
			bestya = adjy;
			bestadjsx = sar;
			bestadjsy = sai;
			bestadjsxa = adjsx;
			bestadjsya = adjsy;
		}
	}
	printf( "Best: %d %d adjx:%d adjy:%d [adjsx: %d adjsy: %d %d %d] (%f)\n", bestx, besty, bestxa, bestya, bestadjsxa, bestadjsya, bestadjsx, bestadjsy, sqrt(minerror/rstop) );
}


