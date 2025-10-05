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
	int64_t emark = 757275091;

	// emark is something like:
	// 65536×sqrt(1-cos(omega_per_step))

	int rstop = 80;

	int64_t scomp2 = 1062592792ULL*2; // halp.  What should this be?

	// Similar to // https://amycoders.org/tutorials/sintables.html  (The last part of the last section)
	for( i = 0; i <= rstop; i++ )
	{
		int emarkadjust = (((int64_t)a*emark)>>32);
		b = b - emarkadjust;
		printf( "%d %d\n", (a*scomp2*2)>>32, b );
		b = b - emarkadjust;
		a = a + (((int64_t)b*emark)>>32);
	}
}

