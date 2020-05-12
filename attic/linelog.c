#include <stdio.h>


#define GENLINEWIDTH 15
#define GENLINES 8

int genlinelen = 0;
char genlog[GENLINEWIDTH*GENLINES+2] = "log";
int genloglen;
int genloglines;
int firstnewline = -1;

void example_log_function( int readSize, char * buf )
{
//	static og_mutex_t * mt;
//	if( !mt ) mt = OGCreateMutex();
//	OGLockMutex( mt );
	int i;
	for( i = 0; (readSize>=0)?(i <= readSize):buf[i]; i++ )
	{
		char c = buf[i];
		if( c == '\0' ) c = '\n';
		if( ( c != '\n' && genlinelen >= GENLINEWIDTH ) || c == '\n' )
		{
			int k;
			printf( "NEWLINE (c = %d, %d >= %d)\n", c, genlinelen, GENLINEWIDTH );
			genloglines++;
			printf( "LINES: %d\n", genloglines );
			printf( "+++++EWGENLOG FNL: %d\n", firstnewline );
			puts( genlog );
			printf( "GENLOGOK\n" );
			printf( "FNL: %d EGL: %d\n", firstnewline, genloglen );
			if( genloglines >= GENLINES )
			{
				genloglen -= firstnewline+1;
				printf( "NOWGL %d\n", genloglen );
				int offset = firstnewline;
				firstnewline = -1;
				if( firstnewline < 0 ) ;

				for( k = 0; k < genloglen; k++ )
				{
					printf( "[%d] = [%d] (%c) \n", k, k+offset+1, genlog[k+offset+1] );
					if( ( genlog[k] = genlog[k+offset+1] ) == '\n' && firstnewline < 0)
					{
						firstnewline = k;
					}
				}
				genlog[k] = 0;
				genloglines--;
				printf( "POSTSHIFT\n");
				puts( genlog );
				printf( "POSTSHIFT\n");
			}
			printf( "NEWGENLOG\n" );
			printf( "------GENLOGOK\n" );
			genlinelen = 0;
			if( c != '\n' )
			{
				genlog[genloglen+1] = 0;
				genlog[genloglen++] = '\n';
			}
			if( firstnewline < 0 ) firstnewline = genloglen-1;
			puts( genlog );
			printf( "------GENLOGOK2\n" );
		}
		genlog[genloglen+1] = 0;
		genlog[genloglen++] = c;
		if( c != '\n' ) genlinelen++;
	}
}


int main()
{
	example_log_function( -1, "Hello, world how are you doing today this is a test to see what it looks like after two lines.\n\n\nwhat" );
	example_log_function( -1, "Hello, world how are you doing today this is a test to see what it looks like after \n\n     two lines.\n" );
	printf( "%d %d %d\n", genlinelen, genloglen );
	printf( "%s\n", genlog );
}


