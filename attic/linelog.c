#include <stdio.h>


#define GENLINEWIDTH 15
#define GENLINES 8

int genlinelen = 0;
char genlog[(GENLINEWIDTH+1)*(GENLINES+0)+2] = "log";
int genloglen;
int genloglines;
int firstnewline = -1;

void example_log_function( int readSize, char * buf )
{
//	static og_mutex_t * mt;
//	if( !mt ) mt = OGCreateMutex();
//	OGLockMutex( mt );
	int i;
	for( i = 0; (readSize>=0)?(i < readSize):buf[i]; i++ )
	{
		char c = buf[i];
		if( c == '\0' ) c = '\n';
		if( ( c != '\n' && genlinelen >= GENLINEWIDTH ) || c == '\n' )
		{
			int k;
			genloglines++;
			if( genloglines >= GENLINES )
			{
				genloglen -= firstnewline+1;
				int offset = firstnewline;
				firstnewline = -1;

				for( k = 0; k < genloglen; k++ )
				{
					if( ( genlog[k] = genlog[k+offset+1] ) == '\n' && firstnewline < 0)
					{
						firstnewline = k;
					}
				}
				genlog[k] = 0;
				genloglines--;
			}
			genlinelen = 0;
			if( c != '\n' )
			{
				genlog[genloglen+1] = 0;
				genlog[genloglen++] = '\n';
			}
			if( firstnewline < 0 ) firstnewline = genloglen;
		}
		genlog[genloglen+1] = 0;
		genlog[genloglen++] = c;
		if( c != '\n' ) genlinelen++;
	}
}


int main()
{
//	example_log_function( -1, "Hello, world how are you doing today this is a test to see what it looks like after two lines.\n\n\nwhat" );
//	example_log_function( -1, "Hello, world how are you doing today this is a test to see what it looks like after \n\n     two lines.\n" );
	int i;
	for( i = 0; i <1000000; i++ )
	{
		char cts[10];
		char c = (rand()%50)+31;
		if( c == 31 ) c = '\n';
		char c2 = (rand()%50)+31;
		if( c2 == 31 ) c2 = '\n';
		if( c2 == 33 ) c2 = 0;
		char c3 = (rand()%50)+31;
		if( c3 == 31 ) c3 = '\n';
		if( c3 == 33 ) c3 = 0;
		cts[0] = c;
		cts[1] = c2;
		cts[2] = c3;
		cts[3] = 0;
		example_log_function( -1, cts );
		//printf( "%d\n", i );
		//puts( genlog );
	}
	printf( "%d %d %d\n", genlinelen, genloglen );
	printf( "%s\n", genlog );
}


