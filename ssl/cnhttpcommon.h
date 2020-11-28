#ifndef _CNHTTPCOMMON_H
#define _CNHTTPCOMMON_H

int CNURLDecode( char * decodeinto, int maxlen, const char * buf );
int CNURLEncode( char * encodeinto, int maxlen, const char * buf, int buflen );
extern const char tohex1buff[16]; //{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
unsigned char hex1byte( const char data ); //Returns 16 if not valid.
unsigned char hex2byte( const char * data );

#endif

