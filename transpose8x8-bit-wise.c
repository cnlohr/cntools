#include <stdio.h>

// Example 8x8 Transpose Code for transposing bits.

void PrintBin( uint32_t v )
{
	int i;
	for( i = 0; i < 32; i++ )
	{
		if( (i & 7) == 0 ) printf("\n" );
		if( v & 0x80000000 )
			printf( "1" ); else printf( "0" );
		v<<=1;
	}
}

int main()
{
	uint32_t a = 0b00000001000000011000000010000000;
	uint32_t b = 0b10000001100000001000000010000000;
	PrintBin( a );
	PrintBin( b );
	printf( "\n" );

	uint32_t t;
	uint32_t ta;
	uint32_t tb;
	
	
#if 1
	// Reverse Willmore
	ta = ((a & 0xF0F0F0F0) ) | ((b & 0xF0F0F0F0) >> 4);
	tb = ((a & 0x0F0F0F0F) << 4 ) | ((b & 0x0F0F0F0F) );
	ta = (ta & 0xCCCC3333) | (((ta>>14)|(ta<<14)) & 0x3333CCCC);
	tb = (tb & 0xCCCC3333) | (((tb>>14)|(tb<<14)) & 0x3333CCCC);
	ta = (ta & 0xAA55AA55) | (((ta>> 7) & 0x00AA00AA)) | ((ta & 0x00AA00AA) << 7);
	tb = (tb & 0xAA55AA55) | (((tb>> 7) & 0x00AA00AA)) | ((tb & 0x00AA00AA) << 7);
#elif 1
	// Regular Willmore
	ta = (a & 0x0F0F0F0F) | ((b & 0x0F0F0F0F) << 4);
	tb = ((a & 0xF0F0F0F0) >> 4) | (b & 0xF0F0F0F0);
	ta = (ta & 0x3333CCCC) | (((ta>>18) | (ta<<18)) & 0xCCCC3333);
	tb = (tb & 0x3333CCCC) | (((tb>>18) | (tb<<18)) & 0xCCCC3333);
	ta = (ta & 0x55AA55AA) | (((ta>> 9) & 0x00550055)) | ((ta & 0x00550055) << 9);
	tb = (tb & 0x55AA55AA) | (((tb>> 9) & 0x00550055)) | ((tb & 0x00550055) << 9);
#elif 0
	// Stack Overflow https://stackoverflow.com/questions/6930667/what-is-the-fastest-way-to-transpose-the-bits-in-an-8x8-block-on-bits/6932331#6932331
	t = (a ^ (a >> 7)) & 0x00AA00AA; a = a ^ t ^ (t << 7); 
	t = (b ^ (b >> 7)) & 0x00AA00AA; b = b ^ t ^ (t << 7); 
	t = (a ^ (a >>14)) & 0x0000CCCC; a = a ^ t ^ (t <<14); 
	t = (b ^ (b >>14)) & 0x0000CCCC; b = b ^ t ^ (t <<14); 
	ta = (a & 0xF0F0F0F0) | ((b >> 4) & 0x0F0F0F0F);
	tb = ((a << 4) & 0xF0F0F0F0) | (b & 0x0F0F0F0F);
#elif 1
	// Reverse-stack-overflow.
	t = (a ^ (a >> 9)) & 0x00550055; a = a ^ t ^ (t << 9); 
	t = (b ^ (b >> 9)) & 0x00550055; b = b ^ t ^ (t << 9); 
	t = (a ^ (a >>18)) & 0x00003333; a = a ^ t ^ (t <<18); 
	t = (b ^ (b >>18)) & 0x00003333; b = b ^ t ^ (t <<18); 
	ta = (a & 0x0F0F0F0F) | ((b << 4) & 0xF0F0F0F0);
	tb = ((a >> 4) & 0x0F0F0F0F) | (b & 0xF0F0F0F0);
#endif
	PrintBin( ta );
	PrintBin( tb );
	printf( "\n" );

}
