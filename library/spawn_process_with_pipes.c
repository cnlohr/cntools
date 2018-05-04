#include <unistd.h>

int spawn_process_with_pipes( const char * execparam, char * const argv[], int pipefd[3] )
{
	int pipeset[6];
  
	pipe(&pipeset[0]);
	pipe(&pipeset[2]);
	pipe(&pipeset[4]);
	
	int rforkv = fork();
	if (rforkv == 0)
	{
		// child
		close( pipeset[1] );
		close( pipeset[2] );
		close( pipeset[4] );
		dup2(pipeset[0], 0);
		dup2(pipeset[3], 1);
		dup2(pipeset[5], 2);
		close( pipeset[0] );
		close( pipeset[3] );
		close( pipeset[5] );
		execvp( execparam, argv );
	}
	else
	{
		// parent
		pipefd[0] = pipeset[1];
		pipefd[1] = pipeset[2];
		pipefd[2] = pipeset[4];
		close( pipeset[0] );
		close( pipeset[3] );
		close( pipeset[5] );
		return rforkv;
	}
}



int main()
{
	char * argv[3] = { "param1", "param2", 0 };
	int fds[3];

	int procv = spawn_process_with_pipes( "./test", argv, fds );
	printf( "PROCESS: %d\n", procv  );


	usleep( 400000 );
	printf( "Writing to %d\n", fds[0] );
	printf( "  %ld\n", write( fds[0], "Hello!!\n", 8 ) );
	usleep( 400000 );
	char buff[1024];
	int r;
	printf( "READING %d\n", fds[1] );
	r = read( fds[1], buff, 1024 );
	buff[r] = 0;
	int i;
	printf( "1(%d)>", r );
	for( i = 0; i < r; i++ )
	{
		printf( "%02x ", buff[i] );
	}
	printf( "\n%s", buff );
	r = read( fds[2], buff, 1024 );
	buff[r] = 0;
	printf( "\n2(%d)>", r );
	for( i = 0; i < r; i++ )
	{
		printf( "%02x ", buff[i] );
	}
	printf( "\n%s\n", buff );
	int ret;
	printf( "waitpid = %p\n", waitpid(procv, &ret, 0) );
	printf( "Ret: %d\n", WEXITSTATUS(ret) );
	printf( "\n" );
	return 0;
}
