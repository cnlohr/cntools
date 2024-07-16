#include <stdio.h>
#include <sys/time.h>

int main()
{
	struct timeval first, tv;
	gettimeofday(&first, 0);

	while(1)
	{
		int c;
		while( (c = getchar()) )
		{
			putchar(c);
			if( c == '\n' )
			{
				gettimeofday(&tv, 0);
				int secdiff = tv.tv_sec - first.tv_sec;
				int usecdiff = tv.tv_usec - first.tv_usec;
				if( usecdiff < 0 ) { secdiff--; usecdiff+=1000000; }
				printf( "%10d.%06d: ", secdiff, usecdiff );
			}
		}
	}
}

