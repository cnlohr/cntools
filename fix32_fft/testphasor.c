#include <stdint.h>
#include <stdio.h>


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

