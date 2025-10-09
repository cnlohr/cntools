#include <stdint.h>
#include <stdio.h>
#include <math.h>



int main()
{
	int i;

	int rstop = 8192;
	double deltaomega = 3.1415926535*2.0/rstop;

	// for rstop=32
	//dsq = dsq-265650;
	//b = b + 20623729;
	//a = a - 20605845;

	int dsqadj = 0;
	int aaadj = 0;
	int baadj = 0;
	int bmuxadj = -1<<30;

	int bestdsqadj = 0;
	int bestaaadj = 0;
	int bestbaadj = 0;
	int bestbmuxadj = -1<<30;

	int bestseldsqadj = 0;
	int bestselaaadj = 0;
	int bestselbaadj = 0;
	int bestselbmuxadj = -1<<30;

	int usedsq = 0;
	int usea = 0;
	int useb = 0;
	int usemux = 0;

	int iter = 0;
	for( iter = 0; iter < 100000; iter++ )
	{
		bestdsqadj = bestseldsqadj;
		bestaaadj = bestselaaadj;
		bestbaadj = bestselbaadj;
		bestbmuxadj = bestselbmuxadj;

		double besterrorma = 1e30;

		double bvalakN = 0;
		double bvalakD = 0;

		int v0, v1, v2, v3;
		for( v0 = -10; v0 <= 10; v0++ )
		for( v1 = -10; v1 <= 10; v1++ )
		for( v2 = -10; v2 <= 10; v2++ )
		for( v3 = -3; v3 <= 4; v3++ )
		{
			double errorma = 0.0;

			dsqadj = v0 + bestdsqadj;
			aaadj = v1 + bestaaadj;
			baadj = v2 + bestbaadj;
			bmuxadj = v3 + bestbmuxadj;

			if( v3 == 4 )
			{
				//printf( "N/D %f %f\n", bvalakN, bvalakD );
				// Potentially jump to the answer.
				bmuxadj = bvalakN/bvalakD*65536;
				//printf( "MUXVAL %d\n", bmuxadj );
			}

			int32_t d = ((1ULL<<31)-1) * (deltaomega);
			int32_t dsq = ((1ULL<<31)-1) * (deltaomega*deltaomega);

			int32_t a = (1ULL<<30)-1;
			int32_t b = 0;

			dsq = dsq+dsqadj;
			b = b + baadj;
			a = a - aaadj;

			int32_t astart = a;
			int32_t bstart = b;
			int32_t mux = bmuxadj;
			double omega = 0;

		//	printf( "mm %d\n", mux );
		//	float amptune = 2*cos(deltaomega);

			bvalakN = 0;
			bvalakD = 0;

			// Similar to // https://amycoders.org/tutorials/sintables.html  (The Harmonical Oscilator Approach)
			for( i = 0; i <= rstop; i++ )
			{
				// a' = a + b
				// b = b - d^2a'

				a = a + b;
				int32_t aadj = (((a * (int64_t)dsq)>>32));
				b = b - aadj;

				int32_t bval = ((int64_t)b * (mux))>>16;
				if( i == 50 ) printf( "%d %d / targ (%d %d) err (%d %d) %d\n", a, bval, (int)(cos(omega)*1073741824), (int)(sin(omega)*1073741824), (int)(a-cos(omega)*1073741824), (int)(bval-sin(omega)*1073741824), v3 );

				if( b > 500 || b < -500 )
				{
					bvalakN += ((sin(omega)*1073741824) / (b));
					bvalakD++;
				}

				errorma = sqrt((a-cos(omega)*1073741824) * (a-cos(omega)*1073741824) + (bval-sin(omega)*1073741824) * (bval-sin(omega)*1073741824));
				b = b - aadj;

				omega += deltaomega;
			}

		//	printf( "N/D %f\n", bvalakN/bvalakD);

			if( errorma < besterrorma )
			{
				usedsq = dsq;
				usea = astart;
				useb = bstart;
				usemux = mux;


				besterrorma = errorma;
				bestseldsqadj = dsqadj;
				bestselaaadj = aaadj;
				bestselbaadj = baadj;
				bestselbmuxadj = bmuxadj;
				//printf( "%4d %4d %4d %4d  %f\n", bestseldsqadj, bestselaaadj, bestselbaadj, bestselbmuxadj, besterrorma );
			}
		}

		printf( "%d %d %d %d %f\n", usedsq, usea, useb, usemux, besterrorma );
	}

}


#if 0

int main()
{
	int i;
	float a = -1000000 * sin(0.801/2.0); // TUNE ME
	float b = 1000000;

	int rstop = 40;
	double omega = 0.0;

	float d = 3.14159*2.0/8.0;           // TUNE ME
	for( i = 0; i <= rstop; i++ )
	{
		a = a + b * d * 0.5;
		printf( "%f,%f\n", a, b );        // TUNE AMP
		a = a + b * d * 0.5;
		b = b - a * d;

		omega += 0.05;
	}
}

#endif

#if 0
int main()
{
	int i;
	int a = 0;
	int b = 1073741823; // 2^30-1

	// Value -> Omega
	// With 16-bit
	//  32768 = 0.7227 
	//  16384 = 0.3554  scomp2 = 91217
	//   8192 = 0.1770  scomp2 = 92315
	//
	// for 32-bit only
	// 757274467 = 0.25 omega scomp2 = 3013304584
	// 151834210 = 0.05 omega
	int64_t emark = 151834210;

	// emark is something like:
	// 65536×sqrt(1-cos(omega_per_step))

	int rstop = 80;

	int64_t scomp2 = 3036051499ULL; // halp.  What should this be?

	double omega = 0.0;

	// Similar to // https://amycoders.org/tutorials/sintables.html  (The last part of the last section)
	for( i = 0; i <= rstop; i++ )
	{
		int emarkadjust = (((int64_t)a*emark)>>32);
		b = b - emarkadjust;

		int sval = (a*scomp2*2)>>32;
		printf( "%d %d / %d %d (%d %d)\n", sval, b, (int)(sin(omega)*1073741824), (int)(cos(omega)*1073741824), (int)(sval-sin(omega)*1073741824), (int)(b-cos(omega)*1073741824) );
		b = b - emarkadjust;
		a = a + (((int64_t)b*emark)>>32);

		omega += 0.05;
	}
}
#endif

