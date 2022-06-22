#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "http_bsd.h"
#include "cnhttp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


// Here is the beginning of the implementation.


static void huge()
{
	uint8_t i = 0;

	DataStartPacket();
/*
	// Mode 1: The fully cross-platform way.  (~1 Gbit/s)
	do
	{
		PushByte( 0 );
		PushByte( i );
	} while( ++i ); //Tricky:  this will roll-over to 0, and thus only execute 256 times.
*/

/*
	// Mode 2: Assuming the BSD Sockets backend. (~1.7 Gbit/s)
	uint8_t buffer[CNHTTP_MTU] = { 0 };
	memcpy( databuff_ptr, buffer, sizeof( buffer ) );
 	databuff_ptr += CNHTTP_MTU;
*/

	// Mode 3: Just directly access the socket. (~21 Gbit/s)
	int socket_id = curhttp-HTTPConnections;
	int sock = http_bsd_sockets[socket_id];

	if( TCPCanSend( sock, 65536 ) )
	{
		uint8_t buffer[65536] = { 0 };
		send( sock, buffer, 65536, MSG_NOSIGNAL );
	}

	EndTCPWrite( curhttp->socket );
}


static void echo()
{
	char mydat[128];
	int len = URLDecode( mydat, 128, curhttp->pathbuffer+8 );

	DataStartPacket();
	PushBlob( mydat, len );
	EndTCPWrite( curhttp->socket );

	curhttp->state = HTTP_WAIT_CLOSE;
}

static void issue()
{
	uint8_t  __attribute__ ((aligned (32))) buf[CNHTTP_MTU];
	int len = URLDecode( buf, CNHTTP_MTU, curhttp->pathbuffer+9 );
	printf( "%d\n", len );
//	int r = issue_command(buf, CNHTTP_MTU, buf, len );
	int r = 10;
	if( r > 0 )
	{
		printf( "BD: %d\n", r );
		DataStartPacket();
		PushBlob( buf, r );
		EndTCPWrite( curhttp->socket );
	}
	curhttp->state = HTTP_WAIT_CLOSE;
}


void HTTPCustomCallback( )
{
	if( curhttp->rcb )
		((void(*)())curhttp->rcb)();
	else
		curhttp->isdone = 1;
}


//Close of curhttp happened.
void CloseEvent()
{
}

static void WSEchoData(  int len )
{
	char cbo[len];
	int i;
	for( i = 0; i < len; i++ )
	{
		cbo[i] = WSPOPMASK();
	}
	WebSocketSend( cbo, len );
}



static void WSCommandData(  int len )
{
	uint8_t  __attribute__ ((aligned (32))) buf[CNHTTP_MTU];
	int i;

	for( i = 0; i < len; i++ )
	{
		buf[i] = WSPOPMASK();
	}

	//i = issue_command(buf, CNHTTP_MTU, buf, len );
	if( i < 0 ) i = 0;

	WebSocketSend( buf, i );
}



void NewWebSocket()
{
	if( strcmp( (const char*)curhttp->pathbuffer, "/d/ws/echo" ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSEchoData;
	}
	else if( strncmp( (const char*)curhttp->pathbuffer, "/d/ws/issue", 11 ) == 0 )
	{
		curhttp->rcb = 0;
		curhttp->rcbDat = (void*)&WSCommandData;
	}
	else
	{
		curhttp->is404 = 1;
	}
}




void WebSocketTick()
{
	if( curhttp->rcb )
	{
		((void(*)())curhttp->rcb)();
	}
}

void WebSocketData( int len )
{
	if( curhttp->rcbDat )
	{
		((void(*)( int ))curhttp->rcbDat)(  len ); 
	}
}

void HTTPCustomStart( )
{
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/huge", 7 ) == 0 )
	{
		curhttp->rcb = (void(*)())&huge;
		curhttp->bytesleft = 0xffffffff;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/echo?", 8 ) == 0 )
	{
		curhttp->rcb = (void(*)())&echo;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/issue?", 9 ) == 0 )
	{
		curhttp->rcb = (void(*)())&issue;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/exit", 7 ) == 0 )
	{
		exit(0);
	}
	else
	{
		curhttp->rcb = 0;
		curhttp->bytesleft = 0;
	}
	curhttp->isfirst = 1;
	HTTPHandleInternalCallback();
}


int main()
{
	RunHTTP( 8888 );

	while(1)
	{
		TickHTTP();
	}
}

