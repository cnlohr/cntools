//Don't fortget to apt-get install libssl-dev
//#include <openssl/applink.c>

#if defined(WINDOWS) || defined( WIN32 ) || defined( WIN64 )
#include "winsock2.h"
#include <string.h>
#define MSG_NOSIGNAL 0
#else
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "cnsslclient.h"

static uint8_t did_ssl_init;

struct cnsslinternal
{
#if !defined(WINDOWS) && !defined( WIN32 ) && !defined( WIN64 )
	SSL_CTX * ctx;
	SSL     * c;
#else
	int ctx, c;
#endif
	int       sock;
};

cnsslclient CNSSLMakeClient( const char * hostname, int portno, int encrypt )
{
	struct sockaddr_in serveraddr;
	struct hostent *server;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( sockfd < 0 )
	{
		fprintf( stderr, "Can't create socket.\n" );
		return 0;
	}

	server = gethostbyname(hostname);
	if (server == NULL) {
		close( sockfd );
		return 0;
	}

	memset((char *) &serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	memcpy((char *)&serveraddr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
	serveraddr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) 
	{
		fprintf( stderr, "Connection failed.\n" );
		close( sockfd );
		return 0;
	}

	struct cnsslinternal * intl = malloc( sizeof( struct cnsslinternal ) );

#if defined(WINDOWS) || defined( WIN32 ) || defined( WIN64 )
	if( encrypt )
	{
		fprintf( stderr, "ERROR: SSL Not currently supported in Win32\n" );
		return 0;
	}
#else
	if( encrypt )
	{
		SSL_CTX *sslctx = 0;
		SSL *cSSL = 0;
		if( !did_ssl_init )
		{
			SSL_load_error_strings();
			SSL_library_init();
			SSL_load_error_strings();
			SSL_library_init();
			did_ssl_init = 1;
		}

		sslctx = SSL_CTX_new( SSLv23_client_method());
		SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);

		cSSL = SSL_new(sslctx);
		SSL_set_fd( cSSL, sockfd );
		SSL_set_app_data( cSSL, encrypt );
		if( SSL_connect(cSSL) <= 0 )
		{
			fprintf( stderr, "SSL Attach failed.\n" );
			free( intl );
			return 0;
		}
		intl->c = cSSL;
		intl->ctx = sslctx;
	}
	else
#endif
	{
		intl->c = 0;
		intl->ctx = 0;
	}

	intl->sock = sockfd;
	return intl;
}

void CNSSLClose( cnsslclient c )
{
	struct cnsslinternal * intl = c;

#if defined(WINDOWS) || defined( WIN32 ) || defined( WIN64 )
	closesocket( intl->sock );
#else
	if( intl->c )   SSL_shutdown(intl->c);
	close( intl->sock );
	if( intl->c )   SSL_free(intl->c);
	if( intl->ctx ) SSL_CTX_free( intl->ctx );
#endif
	free( intl );
}

int CNSSLWrite( cnsslclient c, const void * data, int length )
{
	struct cnsslinternal * intl = c;
#if !defined(WINDOWS) && !defined( WIN32 ) && !defined( WIN64 )
	if( intl->c )
	{
		return SSL_write(intl->c, data, length);
	}
	else
#endif
	{
		return send( intl->sock, data, length, MSG_NOSIGNAL );
	}
}

int CNSSLRead( cnsslclient c, void * data, int maxlen )
{
	struct cnsslinternal * intl = c;
#if !defined(WINDOWS) && !defined( WIN32 ) && !defined( WIN64 )
	if( intl->c )
	{
		return SSL_read(intl->c, data, maxlen);
	}
	else
#endif
	{
		return recv( intl->sock, data, maxlen, 0 );
	}
}

void CNSSLShutdown()
{
#if !defined(WINDOWS) && !defined( WIN32 ) && !defined( WIN64 )
	if( did_ssl_init )
	{
		ERR_free_strings();
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
	    ERR_remove_thread_state(0);
	}
#endif
	did_ssl_init = 0;
}
