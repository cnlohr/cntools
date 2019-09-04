//
//TODO: Actually make work in Windows.
//

#ifndef _TCC_CRASH_H
#define _TCC_CRASH_H

#include <stdint.h>
#include <setjmp.h>

//Things this needs to do:
//1: Provide a "checkpoint" for caught system exceptions
//2: Create a tree of all known symbols.
//3: Process a crash dump.

void tcccrash_install();
int  tcccrash_checkpoint();
#define  tcccrash_checkpoint() (sigsetjmp( tcccrash_getcheckpoint()->jmpbuf, -1))
char * tcccrash_getcrash();
void tcccrash_closethread();

typedef struct tcccrash_syminfo_t
{
	void  * tag;
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
	char * lastcrash;
	intptr_t * btrace;	//Not needed in Windows?
} tcccrashcheckpoint;

//TCCCrash will handle deleting the pointer from here if need be.
tcccrashcheckpoint * tcccrash_getcheckpoint();
void tcccrash_symset( void * tag, tcccrash_syminfo * symadd );
void tcccrash_deltag( void * tag );
tcccrash_syminfo * tcccrash_symget( intptr_t address );


#endif


