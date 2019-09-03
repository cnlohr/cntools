//
//TODO: Actually make work in Windows.
//

#ifndef _TCC_CRASH_H
#define _TCC_CRASH_H

//Things this needs to do:
//1: Provide a "checkpoint" for caught system exceptions
//2: Create a tree of all known symbols.
//3: Process a crash dump.

void tcccrash_install();
int  tcccrash_checkpoint();
char * tcccrash_getcrash();
void tcccrash_closethread();

typedef struct tcccrash_syminfo_t
{
	void  * tag;
	const char * symbol;
	const char * path;
	intptr_t address;
	int size;
} tcccrash_syminfo;

//TCCCrash will handle deleting the pointer from here if need be.
void tcccrash_symset( void * tag, tcccrash_syminfo * symadd );
void tcccrash_deltag( void * tag );
int tcccrash_symget( intptr_t address, tcccrash_syminfo ** symadd );


#if 0


#include <setjmp.h>


#if defined( TCC )
#define TCCRASHTHREADLOCAL 0
#elif defined( __GNUC__ ) || defined( __clang__ )
#define TCCRASHTHREADLOCAL __thread
#elif defined( _MSC_VER )
#define TCCRASHTHREADLOCAL __declspec(thread)
#else
#error No thread-local storage defined for this platform
#endif

struct tcccrashcheckpoint_t
{
} tcccrashcheckpoint;

#if TCCRASHTHREADLOCAL == 0

extern TCCRASHTHREADLOCAL jmp_buf jblocal;

#define TCC_CRASH_CHECKPOINT() \
	( setjmp( 

//Returns 0 if no crash happened.
int tcccrash_checkpoint();

#endif

#endif

