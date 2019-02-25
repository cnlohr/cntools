#include "http_bsd.h"
#ifdef WIN32
#include <winsock2.h>
#define socklen_t uint32_t
#define SHUT_RDWR SD_BOTH
#define MSG_NOSIGNAL 0
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/in.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
uint16_t htons(uint16_t hostshort);

#include <stdio.h>
#include <string.h>
#include <time.h>

static  int serverSocket;

uint8_t * databuff_ptr;
uint8_t   databuff[1536];
int cork_binary_rx;


#ifndef HTTP_POLL_TIMEOUT
#define HTTP_POLL_TIMEOUT 100 //100ms timeout.
#endif


#define DESTROY_SOCKETS_LIST 200
int destroy_sockets[DESTROY_SOCKETS_LIST];
int destroy_socket_head = 0;
int sockets[HTTP_CONNECTIONS];

void et_espconn_disconnect( int socket )
{
	shutdown( socket, SHUT_RDWR );
	int i;
	//printf( "Shut: %d\n", socket );
	for( i = 0; i < HTTP_CONNECTIONS; i++ )
	{
		if( sockets[i] == socket )
		{
			http_disconnetcb( i );
			sockets[i] = 0;
		}
	}

	if( destroy_sockets[destroy_socket_head] ) closesocket( destroy_sockets[destroy_socket_head] );
	destroy_sockets[destroy_socket_head] = socket;
	destroy_socket_head = (destroy_socket_head+1)%DESTROY_SOCKETS_LIST;

}

void DataStartPacket()
{
	databuff_ptr = databuff;
}

void PushByte( uint8_t c )
{
	if( databuff_ptr - databuff + 1 >= sizeof( databuff ) ) return;
	*(databuff_ptr++) = c;
}

void PushBlob( const uint8_t * data, int len )
{
	if( databuff_ptr - databuff + len >= sizeof( databuff ) ) return;
	memcpy( databuff_ptr, data, len );
	databuff_ptr += len;
}

void PushString( const char * data )
{
	int len = strlen( data );
	if( databuff_ptr - databuff + len >= sizeof( databuff ) ) return;
	memcpy( databuff_ptr, data, len );
	databuff_ptr += len;
}

int TCPCanSend( int socket, int size )
{
	fd_set write_fd_set;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO (&write_fd_set);
	FD_SET (socket, &write_fd_set);

	int r = select (FD_SETSIZE, NULL, &write_fd_set, NULL, &tv);
	if (r < 0)
    {
		perror ("select");
		return -1;
    }
	return r;
}


int TCPCanRead( int sock )
{
	fd_set read_fd_set;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO (&read_fd_set);
	FD_SET (sock, &read_fd_set);

	int r;
	r = select (FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);
	if (r < 0)
    {
		perror ("select");
		return -1;
    }
	return r;
}

int TCPException( int sock )
{
	int error_code;
	int error_code_size = sizeof(error_code);
	getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size);
	if( error_code >= 0 ) return 0;
	else return 1;
}

int TCPDoneSend( int socket )
{
	return TCPCanSend( socket, 1 );
}


int EndTCPWrite( int socket )
{
	int r = send( socket, databuff, databuff_ptr-databuff, MSG_NOSIGNAL );
	databuff_ptr = databuff;
	return r;
}

void TermHTTPServer()
{
	shutdown( serverSocket, SHUT_RDWR );
}

int TickHTTP()
{
	int i;
	struct pollfd allpolls[HTTP_CONNECTIONS+1];
	short mappedhttp[HTTP_CONNECTIONS+1];
	if( serverSocket == 0 ) return -1;

	while(1)
	{
		static double last;
		double now;
#ifdef WIN32
		static LARGE_INTEGER lpf;
		LARGE_INTEGER li;

		if( !lpf.QuadPart )
		{
			QueryPerformanceFrequency( &lpf );
		}

		QueryPerformanceCounter( &li );
		now = (double)li.QuadPart / (double)lpf.QuadPart;
#else
		struct timeval tv;
		gettimeofday( &tv, 0 );
		now = ((double)tv.tv_usec)/1000000. + (tv.tv_sec);
#endif
		double dl = now - last;
		if( dl > .1 )
		{
			int i;
			HTTPTick( 1 );
			last = now;
		}
		else
		{
			HTTPTick( 0 );
		}

		int pollct = 1;
		allpolls[0].fd = serverSocket;
		allpolls[0].events = POLLIN;
		for( i = 0; i < HTTP_CONNECTIONS;i ++)
		{
			if( !sockets[i] || HTTPConnections[i].state == 0 ) continue;
			allpolls[pollct].fd = sockets[i];
			allpolls[pollct].events = POLLIN | (HTTPConnections[i].send_pending?POLLOUT:0);
			mappedhttp[pollct] = i;
			pollct++;
		}

		//Do something to watch all currently-waiting sockets.
		poll( allpolls, pollct, HTTP_POLL_TIMEOUT ); 

		//If there's faults, bail.
		if( allpolls[0].revents & (POLLERR|POLLHUP) )
		{
			closesocket( serverSocket );
			for( i = 0; i < HTTP_CONNECTIONS;i ++)
			{
				if( sockets[i] ) closesocket( sockets[i] );
			}
			break;
		}
		if( allpolls[0].revents & POLLIN )
		{
			struct   sockaddr_in tin;
			socklen_t addrlen  = sizeof(tin);
			memset( &tin, 0, addrlen );
			int tsocket = accept( serverSocket, (struct sockaddr *)&tin, &addrlen );

#ifdef WIN32
			struct linger lx;
			lx.l_onoff = 1;
			lx.l_linger = 0;

			//Disable the linger here, too.
			setsockopt( tsocket, SOL_SOCKET, SO_LINGER, (const char*)&lx, sizeof( lx ) );
#else
			struct linger lx;
			lx.l_onoff = 1;
			lx.l_linger = 0;

			//Disable the linger here, too.
			setsockopt( tsocket, SOL_SOCKET, SO_LINGER, &lx, sizeof( lx ) );
#endif

			int r = httpserver_connectcb( tsocket );

			if( r == -1 )
			{
				closesocket( tsocket );
			}
			else
			{
				sockets[r] = tsocket;
			}
		}
		for( i = 1; i < pollct; i++)
		{
			int wc = mappedhttp[i];
			if( allpolls[i].revents & (POLLERR|POLLHUP) )
			{
				http_disconnetcb( wc );
				closesocket( sockets[wc] );
				sockets[wc] = 0;
			}
			else if( allpolls[i].revents & POLLIN )
			{
				int dco = HTTPConnections[i].corked_data_place;
				uint8_t data[8192];
				memcpy( data, HTTPConnections[i].corked_data, dco );
				int len = recv( sockets[wc], data+dco, 8192-dco, 0 );
				if( len )
				{
					cork_binary_rx = 0;
					http_recvcb( wc, data, len+dco );
					if( cork_binary_rx )
					{
						int to_cork = len;
						if( to_cork > sizeof( HTTPConnections[i].corked_data ) + HTTPConnections[i].corked_data_place )
						{
							http_disconnetcb( wc );
							closesocket ( sockets[wc] );
							sockets[wc] = 0;
							fprintf( stderr, "Error: too much data to buffer on websocket\n" );
						}
						else
						{
							memcpy( HTTPConnections[i].corked_data + dco, data + dco, to_cork );
							HTTPConnections[i].corked_data_place += to_cork;
						}
					}
					else
					{
						HTTPConnections[i].corked_data_place = 0;
					}
				}
				else
				{
					http_disconnetcb( wc );
					closesocket( sockets[wc] );
					sockets[wc] = 0;
				}
			}
		}
	}
	return 0;
}


int RunHTTP( int port )
{
#ifdef WIN32
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        fprintf( stderr, "WSAStartup failed with error: %d\n", err);
        return 1;
    }
}
#endif


	int acceptfaults = 0;
	struct	sockaddr_in sin;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	//Make sure the socket worked.
	if( serverSocket == -1 )
	{
		fprintf( stderr, "Error: Cannot create socket.\n" );
		return -1;
	}

	//Disable SO_LINGER (Well, enable it but turn it way down)
#ifdef WIN32
	struct linger lx;
	lx.l_onoff = 1;
	lx.l_linger = 0;
	setsockopt( serverSocket, SOL_SOCKET, SO_LINGER, (const char *)&lx, sizeof( lx ) );

	//Enable SO_REUSEADDR
	int reusevar = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reusevar, sizeof(reusevar));
#else
	struct linger lx;
	lx.l_onoff = 1;
	lx.l_linger = 0;
	setsockopt( serverSocket, SOL_SOCKET, SO_LINGER, &lx, sizeof( lx ) );

	//Enable SO_REUSEADDR
	int reusevar = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reusevar, sizeof(reusevar));
#endif
	//Setup socket for listening address.
	memset( &sin, 0, sizeof( sin ) );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons( port );

	//Actually bind to the socket
	if( bind( serverSocket, (struct sockaddr *) &sin, sizeof( sin ) ) == -1 )
	{
		fprintf( stderr, "Could not bind to socket: %d\n", port );
		closesocket( serverSocket );
		serverSocket = 0;
		return -1;
	}

	//Finally listen.
	if( listen( serverSocket, 5 ) == -1 )
	{
		fprintf(stderr, "Could not lieten to socket.");
		closesocket( serverSocket );
		serverSocket = 0;
		return -1;
	}

}






void Uint32To10Str( char * out, uint32_t dat )
{
	int tens = 1000000000;
	int val;
	int place = 0;

	while( tens > 1 )
	{
		if( dat/tens ) break;
		tens/=10;
	}

	while( tens )
	{
		val = dat/tens;
		dat -= val*tens;
		tens /= 10;
		out[place++] = val + '0';
	}

	out[place] = 0;
}

//from http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
static const char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                      'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                      '4', '5', '6', '7', '8', '9', '+', '/'};

static const int mod_table[] = {0, 2, 1};

void my_base64_encode(const unsigned char *data, unsigned int input_length, uint8_t * encoded_data )
{

	int i, j;
    int output_length = 4 * ((input_length + 2) / 3);

    if( !encoded_data ) return;
	if( !data ) { encoded_data[0] = '='; encoded_data[1] = 0; return; }

    for (i = 0, j = 0; i < input_length; ) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

	encoded_data[j] = 0;
}

uint8_t hex1byte( char c )
{
	if( c >= '0' && c <= '9' ) return c - '0';
	if( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
	if( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
	return 0;
}

uint8_t hex2byte( const char * c )
{
	return (hex1byte(c[0])<<4) | (hex1byte(c[1]));
}

