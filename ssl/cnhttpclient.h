#ifndef _CNHTTPCLIENT_H
#define _CNHTTPCLIENT_H


struct cnhttpclientrequest
{
	const char * host;  //0, or a host (USUALLY 0!!!).  If just a host, will need port to be set.
	int port; //If 0, will decide based off of http(s) in URL
	int is_secure;

	const char * URL; //May be "http(s)://...com/" or just "/index.html" if host + port is specified.
	const char * AddedHeaders; //Terminate each header (EXCEPT FOR LAST) with \r\n

	void * AuxData;    //For Websockets, this is the "Origin" URL.  Otherwise, it's Post data.
	int AuxDataLength;
};

struct cnhttpclientresponse
{
	int code; //0 = connection issues.  1 = Bad URL/Input data, 2 = Bad response from server.... 1xx, 3xx, 4xx, 5xx = faults of various kids.
	char * fullheader;
	int payloadlen;
	char * payload;
	void * underlying_connection;
};

struct cnhttpclientresponse * CNHTTPClientTransact( struct cnhttpclientrequest * r );
void CNHTTPClientCleanup( struct cnhttpclientresponse * c );

int CNHTTPClientWSSend( struct cnhttpclientresponse * conn, const char * data, int length );
int CNHTTPClientWSRecv( struct cnhttpclientresponse * conn, char * data, int length );

#endif

