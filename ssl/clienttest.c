#include "cnsslclient.h"
#include "cnhttpclient.h"

#include <stdio.h>
#include <string.h>

int main()
{
	int i;
#if 0
	cnsslclient c;
	for( i = 0; i < 10; i++ )
	{
		c = CNSSLMakeClient( "www.google.com", 443, 1 );
		if( !c )
		{
			fprintf( stderr, "Error connecting\n" );
			return -1;
		}

		const char * st = "GET / HTTP/1.1\r\nHost: www.google.com\r\n\r\n";
		CNSSLWrite( c, st, strlen(st) );

		char buff[8192];
		int rec = CNSSLRead( c, buff, sizeof( buff ) );
		printf( "(%d) : (%s)\n", rec, buff );

		CNSSLClose( c );

	}
#endif

#if 0
	struct cnhttpclientrequest req;
	memset( &req, 0, sizeof( req ) );
	req.host = 0;
	req.port = 0;
	req.URL = "https://www.google.com/";
	req.AddedHeaders = 0;
	req.AuxData = 0;
	req.AuxDataLength = 0;

	for( i = 0; i < 10; i++ )
	{
		struct cnhttpclientresponse * r = CNHTTPClientTransact( &req );
		printf( "%d: %d: %s\n", r->code, r->payloadlen, r->payload );
		CNHTTPClientCleanup( r );
	}
#endif

#if 0
	struct cnhttpclientrequest req;
	memset( &req, 0, sizeof( req ) );
	req.host = 0;
	req.port = 0;
	req.URL = "https://discordapp.com/api/v6/users/@me";
	req.AddedHeaders = "Authorization: Bot ++++++++++++++++++++-+++++++++++++++++++\r\nUser-Agent: DiscordBot (https://github.com/cnlohr,0)";
	req.AuxData = 0;
	req.AuxDataLength = 0;
	struct cnhttpclientresponse * r = CNHTTPClientTransact( &req );
	printf( "%d: %d: %s\n", r->code, r->payloadlen, r->payload );
	CNHTTPClientCleanup( r );
#endif

#if 0
	//Get a bunch of messages

	struct cnhttpclientrequest req;
	memset( &req, 0, sizeof( req ) );
	req.host = 0;
	req.port = 0;
	req.URL = "https://discordapp.com/api/v6/channels/262099742069227520/messages?before=301797742127218688&limit=100";
	req.AddedHeaders = "Authorization: Bot ++++++++++++++++++++-+++++++++++++++++++\r\nUser-Agent: DiscordBot (https://github.com/cnlohr,0)";
	req.AuxData = 0;
	req.AuxDataLength = 0;
	struct cnhttpclientresponse * r = CNHTTPClientTransact( &req );
	printf( "%d: %d: %s\n", r->code, r->payloadlen, r->payload );
	CNHTTPClientCleanup( r );
#endif


#if 0
	struct cnhttpclientrequest req;
	memset( &req, 0, sizeof( req ) );
	req.host = 0;
	req.port = 0;
	req.URL = "https://discordapp.com/api/v6/gateway/bot";
	req.AddedHeaders = "Authorization: Bot ++++++++++++++++++++-+++++++++++++++++++\r\nUser-Agent: DiscordBot (https://github.com/cnlohr,0)";
	req.AuxData = 0;
	req.AuxDataLength = 0;
	struct cnhttpclientresponse * r = CNHTTPClientTransact( &req );
	printf( "%d: %d: %s\n", r->code, r->payloadlen, r->payload );
	CNHTTPClientCleanup( r );
#endif


#if 1
	struct cnhttpclientrequest req;
	memset( &req, 0, sizeof( req ) );
	req.host = 0;
	req.port = 0;
	req.URL = "wss://gateway.discord.gg/";
	req.AddedHeaders = "Authorization: Bot ++++++++++++++++++++-+++++++++++++++++++\r\nUser-Agent: DiscordBot (https://github.com/cnlohr,0)";
	req.AuxData = 0;
	req.AuxDataLength = 0;
	struct cnhttpclientresponse * r = CNHTTPClientTransact( &req );
	printf( "%d: %d: %s\n", r->code, r->payloadlen, r->payload );
	if( r->code == 101 )
	{
		//In websocket mode.
		const char * cs = "{\"op\": 1,\"d\": 251}";
		char buff[8192];
		CNHTTPClientWSSend( r, cs, strlen( cs ) );
		int rc = CNHTTPClientWSRecv( r, buff, sizeof( buff ));
		printf( "%d: %s\n", rc, buff );
	}


	CNHTTPClientCleanup( r );
#endif

	CNSSLShutdown();
}


