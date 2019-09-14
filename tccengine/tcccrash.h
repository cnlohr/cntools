//
//TODO: Actually make work in Windows.
//

#ifndef _TCC_CRASH_H
#define _TCC_CRASH_H

#include <stdint.h>
#include <setjmp.h>
#include "tinycc/libtcc.h"

//Things this needs to do:
//1: Provide a "checkpoint" for caught system exceptions
//2: Create a tree of all known symbols.
//3: Process a crash dump.

void tcccrash_install();
//tcccrash_checkpoint returns 0 on success, nonzero on fault.
#if defined( WIN32 ) || defined( WINDOWS ) || defined( WIN64 )
#define  tcccrash_checkpoint() ( tcccrash_getcheckpoint()->can_jump = 1, setjmp( tcccrash_getcheckpoint()->jmpbuf, -1 ) )
#else
#define  tcccrash_checkpoint() ( tcccrash_getcheckpoint()->can_jump = 1, sigsetjmp( tcccrash_getcheckpoint()->jmpbuf, -1 ) )
#endif
#define tcccrash_nullifycheckpoint() { tcccrash_getcheckpoint()->can_jump = 0; }
char * tcccrash_getcrash();
void tcccrash_closethread();

typedef struct tcccrash_syminfo_t
{
	intptr_t tag;
	char * name;
	char * path;
	intptr_t address;
	int size;
} tcccrash_syminfo;


typedef struct tcccrashcheckpoint_t
{
	jmp_buf jmpbuf;
	int did_crash;
	int can_jump;
	int in_crash_handler;
	char * lastcrash;
	void ** btrace;	//Not needed in Windows?
	uint8_t * signalstack; //Not used in windows? Also, unused in non-root instances.
} tcccrashcheckpoint;


#ifndef LIBTCC_H
typedef struct TCCState TCCState;
#endif


//TCCCrash will handle deleting the pointer from here if need be.
tcccrashcheckpoint * tcccrash_getcheckpoint();
void tcccrash_symset( intptr_t tag, tcccrash_syminfo * symadd );
void tcccrash_deltag( intptr_t tag );
tcccrash_syminfo * tcccrash_symget( intptr_t address );
void * tcccrash_symaddr( intptr_t tag, const char * symbol );
void tcccrash_symtcc( const char * file, TCCState * tcc );

#endif


