//XXX WARNING: THIS CODE IS BARELY TESTED!!!


#include "cnsslclient.h"
#include "cnhttpcommon.h"
#include "cnhttpclient.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#define MAX_RESPONSE_LENGTH 25000000

struct cnhttpclientresponse * CNHTTPClientTransact( struct cnhttpclientrequest * r )
{
	int i;
	struct cnhttpclientresponse * ret = malloc( sizeof( struct cnhttpclientresponse  ) );

	int selport = r->port;
	char * selhost = r->host?strdup( r->host ):0;
	char * reqhost = r->host?strdup( r->host ):0;

	const char * urlptr = r->URL;
	int is_secure = r->is_secure;

	int is_http_string = 0;
	int is_websocket = 0;

	memset( ret, 0, sizeof( *ret ) );

	     if( strncmp( urlptr, "ws://", 5 ) == 0 )    { is_websocket = 1; is_http_string = 5; }
	else if( strncmp( urlptr, "wss://", 6 ) == 0 )   { is_websocket = 1; is_secure = 1; is_http_string = 6; }
	else if( strncmp( urlptr, "http://", 7 ) == 0 )  { is_http_string = 7; }
	else if( strncmp( urlptr, "https://", 8 ) == 0 ) { is_http_string = 8; is_secure = 1; }

	if( is_http_string  )
	{
		urlptr += is_http_string;
		int expectingport = 0;

		for( i = 0; urlptr[i]; i++ )
		{
			if( (expectingport = (urlptr[i] == ':')) || urlptr[i] == '/' )
			{
				if( reqhost )
				{
					free( reqhost );
				}
				reqhost = malloc( i+1 );
				memcpy( reqhost, urlptr, i );
				reqhost[i] = 0;

				if( !selhost )
				{
					selhost = strdup( reqhost );
				}
				if( !expectingport )
					break;
			}
		}

		if( expectingport )
		{
			i++;
			for( ; urlptr[i]; i++ )
			{
				if( urlptr[i] == '/' ) break;
			}
			int pt = atoi( urlptr );
			if( !selport ) selport = pt;
		}
		else if( !selport )
		{
			selport = is_secure?443:80;
		}

		urlptr += i;
	}

	//printf( "Connecting: %s/%s : %d\n", reqhost, selhost, selport );

	//urlptr now contains "/index.html"
	//selhost contains the string of the actual hostname to connect to (must be free'd)
	//selport contains the port number to connect to.
	if( !selport || !selhost )
	{
		ret->code = 1;
		if( reqhost ) free( reqhost );
		return ret;
	}

	if( !reqhost ) reqhost = strdup( selhost );

	cnsslclient c = CNSSLMakeClient( selhost, selport, is_secure );
	free( selhost );
	if( !c )
	{
		ret->code = 0;
		if( reqhost )
			free( reqhost );
		return ret;
	}

	//Connected successfully to remote host.
	int eurilen = strlen( urlptr ) * 3;
	char encodeduri[eurilen];
	//int encurilen = CNURLEncode( encodeduri, eurilen, urlptr, strlen( urlptr ) );
	int encurilen = sprintf( encodeduri, "%s", urlptr );
	//encodeduri is a null-terminated string, now.

	if( is_websocket )
	{
		char SecWebKey[17];
		char full_header[encurilen + 300 + strlen(reqhost)];

		int ra = rand();  //A random base64 thingie.
		for( i = 0; i < 16; i++ )
		{
			SecWebKey[i] = (ra & (1<<i))?'a':'b';
		}
		SecWebKey[i] = 0;

		int fhlen = sprintf( full_header, "GET %s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Protocol: chat, superchat\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nOrigin: %s\r\n%s\r\n\r\n", encodeduri, reqhost, SecWebKey, (char*)r->AuxData, r->AddedHeaders?r->AddedHeaders:"" );
		free( reqhost );

		CNSSLWrite( c, full_header, fhlen );
	}
	else
	{
		char full_header[encurilen + 30 + strlen(reqhost) + (r->AddedHeaders?strlen( r->AddedHeaders ):0)];
		int fhlen = sprintf( full_header, "%s %s HTTP/1.1\r\nConnection: close\r\nHost: %s\r\n%s\r\n\r\n", r->AuxData?"POST":"GET", encodeduri, reqhost, r->AddedHeaders?r->AddedHeaders:"" );
		free( reqhost );
		CNSSLWrite( c, full_header, fhlen );
		if( r->AuxData )
			CNSSLWrite( c, r->AuxData, r->AuxDataLength );
	}

	//Wait for response
	char * response = malloc(8192);
	int responselength = 0;
	int responsecode = 2;
	int currentcontent = 0;
	int contentlength = -1;
	int connectionmode = 0;
	int doneheaderat = 0;
	int ischunked = 0;
	int rc;

	while( responselength < MAX_RESPONSE_LENGTH - 8192 && (contentlength < 0 || currentcontent < contentlength ) )
	{
		rc = CNSSLRead( c, response + responselength, 8192 );
		if( rc <= 0 ) break;

		int k = responselength;
		responselength += rc;

		if( !doneheaderat )
		{
			//Check to see if it's the end-of-data.
			for( ; k < responselength; k++ )
			{
				if( k > 3 )
				{
					if( response[k-3] == '\r' && response[k-2] == '\n' && response[k-1] == '\r' && response[k-0] == '\n' )
					{
						doneheaderat = k+1;
						break;
					} 
				}
			}

			if( doneheaderat )
			{
				ret->fullheader = malloc( doneheaderat+1 );
				memcpy( ret->fullheader, response, doneheaderat );
				ret->fullheader[doneheaderat] = 0;

				//Process header to see if we have a content-length, also pull out code.
				char * firstresp = strchr( ret->fullheader, ' ' );
				if( !firstresp ) break;
				firstresp++;
				responsecode = atoi( firstresp );
				char * cl = strcasestr( ret->fullheader, "Content-Length:" );
				if( cl )
				{
					contentlength = atoi( cl + 15 );
				}

				cl = strcasestr( ret->fullheader, "Connection: Close" );
				if( cl )
				{
					connectionmode = 1;
				}
				cl = strcasestr( ret->fullheader, "Transfer-Encoding: chunked" );
				if( cl )
				{
					ischunked = 1;
				}

				currentcontent += responselength - k;
				//puts( ret->fullheader );
				if( responsecode == 101 && is_websocket ) break;
			}
		}
		else
		{
			//Rest of payload.
			currentcontent += rc;
		}

		response = realloc( response, responselength + 8192 );

	}

	ret->code = responsecode;
	if( ischunked )
	{
		ret->payload = malloc( currentcontent + 1);
		int payloadpl = 0;
		//Go through payload, since it's chunked.
		char * splpl = response + doneheaderat;
		char * ospl = splpl;
		do
		{
			int chunklen = 0;
			for( i = 0; i < 10; i++ )
			{
				int c = hex1byte( *(splpl++) );
				if( c < 16 )
					chunklen = (chunklen<<4) | c;
				else
					break;
			}
			if( chunklen == 0 ) break;
			// it's on a \r.  advance.
			splpl+=1;
			if( splpl - ospl + chunklen >= currentcontent ) break;
			memcpy( ret->payload + payloadpl, splpl, chunklen );
			payloadpl += chunklen;
		} while( 1 );
		ret->payload[payloadpl] = 0;
		ret->payloadlen = payloadpl;
	}
	else
	{
		ret->payload = malloc( currentcontent + 1);
		ret->payload[currentcontent] = 0;
		memcpy( ret->payload, response + doneheaderat, currentcontent ); 
		ret->payloadlen = currentcontent;
	}
	free( response );

	if( is_websocket )
	{
		ret->underlying_connection = c;
	}
	else
	{
		CNSSLClose( c );
	}

	return ret;
}

void CNHTTPClientCleanup( struct cnhttpclientresponse * c )
{
	if( c->fullheader ) free( c->fullheader );
	if( c->payload ) free( c->payload );
	if( c->underlying_connection ) free( c->underlying_connection );
	free( c );
}

int CNHTTPClientWSSend( struct cnhttpclientresponse * conn, const char * data, int length )
{
	uint8_t senddata[length+20];
	uint8_t * sppt = senddata;
	*(sppt++) = 0x81; //"send ascii data, and this is one packet"
	if( length > 65535 )
	{
		*(sppt++) = 0x80 | 127;
		*(sppt++) = 0;
		*(sppt++) = 0;
		*(sppt++) = 0;
		*(sppt++) = 0;

		*(sppt++) = length >> 24;
		*(sppt++) = length >> 16;		
		*(sppt++) = length >> 8;
		*(sppt++) = length;		
	}
	else if( length > 125 )
	{
		*(sppt++) = 0x80 | 126;
		*(sppt++) = length >> 8;
		*(sppt++) = length & 0xFF;
	}
	else
	{
		*(sppt++) = 0x80 | length;
	}

	uint32_t mask = 0;//rand();

	mask = htonl( mask );

	*(sppt++) = mask>>24;
	*(sppt++) = mask>>16;
	*(sppt++) = mask>>8;
	*(sppt++) = mask>>0;

	//...Payload
	int i;
	int dwords = (length+3)/4;

	uint32_t * dsp = (uint32_t*)sppt;
	for( i = 0; i < dwords; i++ )
	{
		*(dsp++) = ((uint32_t*)data)[i] ^ mask;
	}
	sppt += length;

	int k;
	int len = sppt - senddata;
//	printf( "%d: ", len );
//	for( k = 0; k < len; k++ )
//	{
//		printf( "%02x ", senddata[k] );
//	}
//	printf( "\n" );

	return CNSSLWrite( conn->underlying_connection, senddata, sppt - senddata );
}

int CNHTTPClientWSRecv( struct cnhttpclientresponse * conn, char * data, int length )
{
	uint8_t * buff = malloc(8192);
	int buffoffset = 0;
	do
	{
		printf( "RXING\n" );
		uint16_t ret = CNSSLRead( conn->underlying_connection, buff + buffoffset, buffoffset + 8192 );
		if( ret <= 0 )
		{
			CNSSLClose( conn->underlying_connection );
			return -1;
		}
		int k;
		printf( "R %d: ", ret );
		for( k = 0; k < ret; k++ )
		{
			printf( "%02x %c ", (buff + buffoffset)[k], (buff + buffoffset)[k] );
		}
		printf( "\n" );

		if( ret < 0 ) return ret;
		buffoffset += ret;
		buff = realloc( buff, buffoffset + 8192 );
		if( buffoffset > 10 )
		{
			int op = buff[0]&0x0f;
			int hasmask = buff[1]&0x80;
			int len1 = buff[1]&0x7f;
			int len = 0;
			uint8_t * buffptr = &buff[2];
			if( len1 < 126 )
			{
				len = len1;
			}
			else if( len1 == 126 )
			{
				len = (*(buffptr++))<<8;
				len |= (*(buffptr++));
			}
			else if( len1 == 127 )
			{
				buffptr += 4;
				len = (*(buffptr++))<<24;
				len |= (*(buffptr++))<<16;
				len |= (*(buffptr++))<<8;
				len |= (*(buffptr++))<<0;
			}
			uint32_t mask = 0;
			if( hasmask )
			{
				mask = (*(buffptr++))<<24;
				mask |= (*(buffptr++))<<16;
				mask |= (*(buffptr++))<<8;
				mask |= (*(buffptr++))<<0;
				mask = htonl( mask );
			}

			if( len > MAX_RESPONSE_LENGTH )
			{
				CNSSLClose( conn->underlying_connection );
				return -1;
			}

			int headerlen = buffptr - buff;

			if( len + headerlen >= buffoffset )
			{
				int i;
				int dwords = (len+3)/4;
				uint32_t * dsp = (uint32_t *)buffptr;
				for( i = 0; i < dwords; i++ )
				{
					*(dsp++) = ((uint32_t*)data)[i] ^ mask;
				}
				buffptr += len;

				if( len > length ) len = length;
				memcpy( data, dsp, len );
				return len;
			}
		}
	} while( 1 );
}


