#ifndef _CNHTTP_H
#define _CNHTTP_H

#include <stdint.h>
#include "mfs.h"

extern struct HTTPConnection * curhttp;
extern uint8_t * curdata;
extern uint16_t  curlen;
extern uint8_t   wsmask[4];
extern uint8_t   wsmaskplace;



uint8_t WSPOPMASK();
#define HTTPPOP (*curdata++)

//You must provide this.
void HTTPCustomStart( );
void HTTPCustomCallback( );  //called when we can send more data
void WebSocketData( int len );
void WebSocketTick( );
void WebSocketNew();
void HTTPHandleInternalCallback( );
uint8_t hex2byte( const char * c );
void NewWebSocket();
void et_espconn_disconnect( int socket );

//Internal Functions
void HTTPTick( uint8_t timedtick ); 
int URLDecode( char * decodeinto, int maxlen, const char * buf );
void WebSocketGotData( uint8_t c );
void WebSocketTickInternal();
void WebSocketSend( uint8_t * data, int size );

//Host-level functions
void my_base64_encode(const unsigned char *data, unsigned int input_length, uint8_t * encoded_data );
void Uint32To10Str( char * out, uint32_t dat );
void http_recvcb(int conn, char *pusrdata, unsigned short length);
void http_disconnetcb(int conn);
int httpserver_connectcb( int socket ); // return which HTTP it is.  -1 for failure
void DataStartPacket();
extern uint8_t * databuff_ptr;
void PushString( const char * data );
void PushByte( uint8_t c );
void PushBlob( const uint8_t * datam, int len );
int TCPCanSend( int socket, int size );
int TCPDoneSend( int socket );
int EndTCPWrite( int socket );

#define HTTP_CONNECTIONS 50
#ifndef MAX_HTTP_PATHLEN
#define MAX_HTTP_PATHLEN 80
#endif
#define HTTP_SERVER_TIMEOUT		500


#define HTTP_STATE_NONE        0
#define HTTP_STATE_WAIT_METHOD 1
#define HTTP_STATE_WAIT_PATH   2
#define HTTP_STATE_WAIT_PROTO  3

#define HTTP_STATE_WAIT_FLAG   4
#define HTTP_STATE_WAIT_INFLAG 5
#define HTTP_STATE_DATA_XFER   7
#define HTTP_STATE_DATA_WEBSOCKET   8

#define HTTP_WAIT_CLOSE        15

struct HTTPConnection
{
	uint8_t  state:4;
	uint8_t  state_deets;

	//Provides path, i.e. "/index.html" but, for websockets, the last 
	//32 bytes of the buffer are used for the websockets key.  
	uint8_t  pathbuffer[MAX_HTTP_PATHLEN];
	uint8_t  is_dynamic:1;
	uint16_t timeout;

	union data_t
	{
		struct MFSFileInfo filedescriptor;
		struct UserData { uint16_t a, b, c; } user;
		struct UserDataPtr { void * v; } userptr;
	} data;

	void * rcb;
	void * rcbDat; //For websockets primarily.
	void * ccb;                          //Close callback (used for websockets, primarily)

	uint32_t bytesleft;
	uint32_t bytessofar;

	uint8_t  is404:1;
	uint8_t  isdone:1;
	uint8_t  isfirst:1;
	uint8_t  keep_alive:1;
	uint8_t  need_resend:1;
	uint8_t  send_pending:1; //If we can send data, we should?

	int socket;
	uint8_t corked_data[4096];
	int corked_data_place;
};

extern struct HTTPConnection HTTPConnections[HTTP_CONNECTIONS];

#endif

