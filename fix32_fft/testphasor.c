#include <stdint.h>
#include <stdio.h>
#include <math.h>



int main()
{
	int i;

	int rstop = 40;
	float desired =  3.14159*2.0/16.0;

	float a = 1000000;// * cos(-desired);
	float b = 0;//1000000 * sin(-desired);

	float d = desired - 0.02003065; // TUNE ME!!!
	float amptune = 2*cos(desired);

	float dsquareddiv2 = d * d / 2.0;

	// Similar to // https://amycoders.org/tutorials/sintables.html  (The Harmonical Oscilator Approach)
	for( i = 0; i <= rstop; i++ )
	{
		a = a + b;
		float aadjust = a * dsquareddiv2;
		b = b - aadjust;
		printf( "%f,%f\n", a, b*amptune );
		b = b - aadjust;
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

