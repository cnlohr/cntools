#ifndef _CNSSLCLIENT_H
#define _CNSSLCLIENT_H

typedef void * cnsslclient;

//Returns 0 if can't connect.  Connections will block on some platforms like Linux/Windows.
cnsslclient CNSSLMakeClient( const char * host, int port, int encrypt );
void CNSSLClose( cnsslclient c );
int CNSSLWrite( cnsslclient c, const void * data, int length );
int CNSSLRead( cnsslclient c, void * data, int length );
void CNSSLShutdown(); //Optional

#endif
