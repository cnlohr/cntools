#ifndef _HTTP_BSD_H
#define _HTTP_BSD_H

#include "cnhttp.h"

//Call this to start your webserver.
int RunHTTP( int port );
int TickHTTP(); //returns -1 if problem.

//For running on a BSD Sockets System
int htsend( int socket, uint8_t * data, int datact );
void et_espconn_disconnect( int socket );
void http_recvcb(int whichhttp, char *pusrdata, unsigned short length);
void http_disconnetcb(int whichhttp);
int httpserver_connectcb( int socket ); // return which HTTP it is.  -1 for failure
void DataStartPacket();
extern uint8_t * databuff_ptr;
void PushBlob( const uint8_t * data, int len );
void PushByte( uint8_t c );
void PushString( const char * data );
int TCPCanSend( int socket, int size );
int TCPDoneSend( int socket );
int EndTCPWrite( int socket );
void TermHTTPServer();

extern int cork_binary_rx;

#endif

