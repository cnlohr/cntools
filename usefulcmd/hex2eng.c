#include <stdio.h>

#define BUFFERSIZE 8192

int main()
{
	char buftor[BUFFERSIZE];
	int value = 0;
	int bufpl = 0;
	int nothex = 0;
	char c;
	while( ( c = getchar() ) != EOF )
	{
		if( c == ',' || c == '\n' )
		{
			if( nothex ) 
			{
				buftor[bufpl] = 0;
				printf( "%s%c", buftor, c );
				fflush( stdout );
				nothex = 0;
				bufpl = 0;
				value = 0;
			}
			else
			{
				printf( "%d", value );
				fflush( stdout );
				value = 0;
				bufpl = 0;
				putchar( c );
			}
		}
		else if( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' ) )
		{
			int cval = 0;
			if( c >= '0' && c <= '9' )
			{
				cval = c - '0';
			}
			else if( c >= 'A' && c <= 'F' )
			{
				cval = c - 'A' + 10;
			}
			else if( c >= 'a' && c <= 'f' )
			{
				cval = c - 'a' + 10;
			}
			value <<= 4;
			value |= cval;
			buftor[bufpl++] = c;
		}
		else if( c == '\t' || c == ' ' )
		{
			buftor[bufpl++] = c;
		}
		else
		{
			nothex = 1;
			buftor[bufpl++] = c;
		}

		if( bufpl >= BUFFERSIZE-1 )
		{
			fprintf( stderr, "ERROR: Buffer overflow.\n" );
			bufpl = 0;
		}
	}
	if( bufpl )
		printf( "%d", value );
	return 0;
}
