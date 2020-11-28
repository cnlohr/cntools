#ifndef _PROPOSED_OSG_H
#define _PROPOSED_OSG_H

//Proposed os_generic functions.
//THIS IS A DRAFT

#ifndef OSG_INLINE
	#ifdef OSG_STATIC
		#define OSG_INLINE static
		#define OSG_IMPLEMENTATION
	#else
		#define OSG_INLINE
	#endif
#endif

#ifndef OSG_IMPLEMENTATION

OSG_INLINE char * OSGReadFileString( FILE * f, int * length );
OSG_INLINE char * OSGLineFromFile( FILE * f );

#else


#define OSGRF_BUFFERSIZE 1024

//Stream functions
//note: `f` benig 0 is handled.  Will just return 0 for a pointer.
//Upon failure, NULL is returned, otherwise malloc's a buffer big enough for the data.
//If f has no bytes left before eof,

//Reads an entire file.
//Closes file when done.
//Length is optional, may be 0.
//OSG_INLINE char * OSGReadFileString( FILE * f, int * length );


//Reads only one line from a file.
//Does not close file when done.
//Does not include \n in stream.
//Does not include \r if present, but will continue reading to \n, or 0.
//OSG_INLINE char * OSGLineFromFile( FILE * f );


OSG_INLINE char * OSGReadFileString( FILE * f, int * length )
{
	int mlen = OSGRF_BUFFERSIZE;
	int len = 0, r;

	if( !f || ferror( f ) || feof( f ) ) return 0;

	char * ret = malloc( mlen );
	do
	{
		r = fread( ret + len, 1, OSGRF_BUFFERSIZE, f );
		if( r == OSGRF_BUFFERSIZE )
			ret = realloc( ret, mlen + OSGRF_BUFFERSIZE );
		if( r < 0 ) break;
		len += r;
	} while( 1 );

	//Tricky: Because of the way we've structured this loop, if the file size lines up perfectly
	//with a buffer boundary, it will automatically be big enough to hold the null terminator.
	ret[len] = 0;
	if( length ) *length = len;
	fclose( f );
	return ret;
}

OSG_INLINE char * OSGLineFromFile( FILE * f )
{
	int c;
	int mlen = OSGRF_BUFFERSIZE;
	int len = 0;

	if( !f || ferror(f) || feof( f ) ) return 0;

	char * ret = malloc( mlen );

	while( 1 )
	{
		c = fgetc( f );
		if( c == EOF || c == 0 || c == '\n' ) break;
		if( c == '\r' ) continue;
		
		ret[len++] = c;
		if( len >= mlen-1 )
		{
			mlen += OSGRF_BUFFERSIZE;
			ret = realloc( ret, mlen );
		}
	}
	ret[len] = 0;
	return ret;
}

#endif //OSG_IMPLEMENTATION

#endif //_PROPOSED_OSG_H


