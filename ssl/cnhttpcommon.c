#include "cnhttpcommon.h"

const char tohex1buff[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

unsigned char hex1byte( const char data )
{
	if( data >= '0' && data <= '9' )
		return (data - '0');
	else if( data >= 'a' && data <= 'f' )
		return (data - 'a'+10);
	else if( data >= 'A' && data <= 'F' )
		return (data - 'A'+10);
	else
		return 16;
}

unsigned char hex2byte( const char * data )
{
	return (hex1byte(data[0])<<4) | (hex1byte(data[1]));
}

int CNURLEncode( char * encodeinto, int maxlen, const char * buf, int buflen )
{
	int i = 0;
	for( ; buf && *buf; buf++ )
	{
		char c = *buf;
		if( c == ' ' )
		{
			encodeinto[i++] = '+';
		}
		else if( c < 46 || c > 126 || c == 96 )
		{
			encodeinto[i++] = '%';
			encodeinto[i++] = tohex1buff[c>>4];
			encodeinto[i++] = tohex1buff[c&15];
			break;
		}
		else
		{
			encodeinto[i++] = c;
		}
		if( i >= maxlen - 3 )  break;
	}
	encodeinto[i] = 0;
	return i;
}

int CNURLDecode( char * decodeinto, int maxlen, const char * buf )
{
	int i = 0;

	for( ; buf && *buf; buf++ )
	{
		char c = *buf;
		if( c == '+' )
		{
			decodeinto[i++] = ' ';
		}
		else if( c == '?' || c == '&' )
		{
			break;
		}
		else if( c == '%' )
		{
			if( *(buf+1) && *(buf+2) )
			{
				decodeinto[i++] = hex2byte( buf+1 );
				buf += 2;
			}
		}
		else
		{
			decodeinto[i++] = c;
		}
		if( i >= maxlen -1 )  break;

	}
	decodeinto[i] = 0;
	return i;
}

