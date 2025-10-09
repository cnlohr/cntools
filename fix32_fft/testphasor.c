#include <stdint.h>
#include <stdio.h>
#include <math.h>



int main()
{
	int i;

	int rstop = 8192;
	double deltaomega = 3.1415926535*2.0/rstop;

	int32_t d = ((1ULL<<31)-1) * (deltaomega);
	int32_t dsq = ((1ULL<<31)-1) * (deltaomega*deltaomega);

	int32_t a = (1ULL<<30)-1;
	int32_t b = 0;

	// for rstop=32
	//dsq = dsq-265650;
	//b = b + 20623729;
	//a = a - 20605845;

	dsq = dsq+1;
	b = b + 315;
	a = a - 315;

	double omega = 0;
//	float amptune = 2*cos(deltaomega);

	// Similar to // https://amycoders.org/tutorials/sintables.html  (The Harmonical Oscilator Approach)
	for( i = 0; i <= rstop; i++ )
	{

		// a' = a + b
		// b = b - d^2a'

		a = a + b;
		int32_t aadj = (((a * (int64_t)dsq)>>32));
		b = b - aadj;

		int32_t bval = ((int64_t)b * -86593307)>>16;
		printf( "%d %d / targ (%d %d) err (%d %d)\n", a, bval, (int)(cos(omega)*1073741824), (int)(sin(omega)*1073741824), (int)(a-cos(omega)*1073741824), (int)(bval-sin(omega)*1073741824) );
		b = b - aadj;

#if 0
#if 0
		int emarkadjust = (((int64_t)a*d)>>32)>>1;
		b = b - emarkadjust;

		int sval = a;
		printf( "%d %d / %d %d (%d %d)\n", sval, b, (int)(sin(omega)*1073741824), (int)(cos(omega)*1073741824), (int)(sval-sin(omega)*1073741824), (int)(b-cos(omega)*1073741824) );
		b = b - emarkadjust;
		a = a + (((int64_t)b*d)>>32);

#endif
		printf( "%d,%d,%d\n", a, b, dsq );
		int32_t badjust = (a * (int64_t)dsq)>>32;
		a = a + ((dsq * (int64_t)b)>>31);
		b = b - badjust;
		printf( "%d,%d\n", a, b );
		b = b - badjust;
#endif
		omega += deltaomega;
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

