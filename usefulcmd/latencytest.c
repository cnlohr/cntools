#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>
#include <errno.h>
#include <string.h>

int threadct;

pthread_t * threads;
sem_t ** sems;
double * unlocktimes;
double * pipetimes;
int * pipes;

float minl, maxl, testLength;
double endTime;

#define HIST_MAX 2000

int timerLatencyHistogram[HIST_MAX];
int sleepLatencyHistogram[HIST_MAX];
int unlockLatencyHistogram[HIST_MAX];
int pipeLatencyHistogram[HIST_MAX];
int realtime_setting;

double GetAbsoluteTime()
{
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
	return ts.tv_sec + ts.tv_nsec * 0.000000001;
}

void * thread( void * v )
{
	int tid = ((uintptr_t)v) / 3;
	int mode = ((uintptr_t)v) % 3;

	// For mimicing Steam Frame
	// THREAD_PRIORITY_REALTIME = 80
	if( realtime_setting )
	{
		struct sched_param par;
		par.sched_priority = sched_get_priority_max( SCHED_RR );
		int err = pthread_setschedparam( pthread_self(), SCHED_RR /* Because it's high priority */, &par );
		if( err )
		{
			fprintf( stderr, "Error: setschedparam returned %d (%s)\n", err, strerror( errno ) );
			exit( -5 );
		}
	}

	if( mode == 0 )
	{
		double unlockTime;
		double now = GetAbsoluteTime();
		do
		{
			now = GetAbsoluteTime();
			double getTimeLatency = GetAbsoluteTime() - now; 
			int timerlatency = getTimeLatency * 1000000;
			if( timerlatency < 0 ) timerlatency = 0;
			if( timerlatency >= HIST_MAX ) timerlatency = HIST_MAX-1;
			__atomic_fetch_add( timerLatencyHistogram + timerlatency, 1, memory_order_relaxed );

			now = GetAbsoluteTime();
			double delay = ( rand() % 1000000 ) * (maxl - minl) * 0.000001 + minl;
			unlockTime = now + delay;
			usleep( (int)(delay*1000000) );
			double now = GetAbsoluteTime();
			unlocktimes[tid] = now;
			sem_post( sems[tid] );
			if( now >= endTime ) break;
			double latency = now - unlockTime;
			int index = latency * 1000000;
			if( index < 0 ) index = 0;
			if( index >= HIST_MAX ) index = HIST_MAX-1;
			__atomic_fetch_add( sleepLatencyHistogram + index, 1, memory_order_relaxed );
		} while( 1 );
		sem_post( sems[tid] );
	}
	else if( mode == 1 )
	{
		// Catcher
		double now;
		do
		{
			sem_wait( sems[tid] );
			now = GetAbsoluteTime();
			double delta = now - unlocktimes[tid];	
			if( now >= endTime ) break;
			int index = delta * 1000000;
			if( index < 0 ) index = 0;
			if( index >= HIST_MAX ) index = HIST_MAX-1;
			__atomic_fetch_add( unlockLatencyHistogram + index, 1, memory_order_relaxed );
			pipetimes[tid] = GetAbsoluteTime();
			write( pipes[tid*2+1], "hello", 5 );
		} while( 1 );
		write( pipes[tid*2+1], "hello", 5 );
	}
	else if( mode == 2 )
	{
		double now;
		do
		{
			char buf[1024];
			read( pipes[tid*2+0], buf, sizeof( buf ) );
			now = GetAbsoluteTime();
			if( now >= endTime ) break;
			double delta = now - pipetimes[tid];
			int index = delta * 1000000;
			if( index < 0 ) index = 0;
			if( index >= HIST_MAX ) index = HIST_MAX-1;
			__atomic_fetch_add( pipeLatencyHistogram + index, 1, memory_order_relaxed );
		} while( 1 );
	}
	return 0;
}

int main( int argc, char ** argv )
{
	if( argc < 6 )
	{
		fprintf( stderr, "Error: Usage: latencytest [number of threads] [realtime (1) or no (0)] [min latency ms] [max latency ms] [test length, seconds]\n" );
		return -5;
	}
	threadct = atoi( argv[1] );
	realtime_setting = atoi( argv[2] );
	minl = atof( argv[3] ) / 1000.0;
	maxl = atof( argv[4] ) / 1000.0;
	testLength = atof( argv[5] );
	if( threadct == 0 || minl < 0.000001 || maxl < 0.000001 || minl > maxl || testLength < 0.001 )
	{
		fprintf( stderr, "Error: Invalid args (%d) (%f) (%f)\n", threadct, minl, maxl );
		return -6;
	}

	threads = malloc( sizeof( pthread_t ) * threadct * 3 );
	sems = malloc( sizeof( sem_t * ) * threadct );
	pipetimes = malloc( sizeof( double ) * threadct );
	unlocktimes = malloc( sizeof( double ) * threadct );
	pipes = malloc( sizeof( int ) * threadct * 2 );
	int i;
	for( i = 0; i < threadct; i++ )
	{
		char semname[128];
		sprintf( semname, "latencytest%03d", i );
		sems[i] = sem_open( semname, O_CREAT, 0666, 0 );
		pipe2( pipes+i*2, O_CLOEXEC ) ;//| O_NONBLOCK );
	}
	
	endTime = GetAbsoluteTime() + testLength;

	for( i = 0; i < threadct * 3; i++ )
	{
		pthread_create( &threads[i], 0, *thread, (void*)(intptr_t)i );
	}
	for( i = 0; i < threadct * 3; i++ )
	{
		pthread_join( threads[i], 0 );
	}

	const double bindiv = 20;
	int accum_timer = 0;
	int accum_sleep = 0;
	int accum_unlock = 0;
	int accum_pipe = 0;
	double lastLine = bindiv;
	for( i = 0; i < HIST_MAX; i++ )
	{
		if( i >= lastLine || i == HIST_MAX-1 )
		{
			printf( "%d, %d, %d, %d, %d\n", i, accum_timer, accum_sleep, accum_unlock, accum_pipe );
			lastLine += bindiv;
			accum_timer = 0;
			accum_sleep = 0;
			accum_unlock = 0;
			accum_pipe = 0;
		}
		accum_timer += timerLatencyHistogram[i];
		accum_sleep += sleepLatencyHistogram[i];
		accum_unlock += unlockLatencyHistogram[i];
		accum_pipe += pipeLatencyHistogram[i];
	}

	return 0;
}


