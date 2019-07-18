#include "symbol_enumerator.h"
#include <stdlib.h>
#include <stdio.h>

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )
#include <windows.h>

#endif

struct SymbolListElement
{
	const char * path;
	const char * name;
	void * location;
	int size;
	struct SymbolListElement * next;
};

static void * tcccr;
static struct SymbolListElement * scl;
int CrashSymEnumeratorCallback( const char * path, const char * name, void * location, long size )
{
	struct SymbolListElement * new = malloc( sizeof( struct SymbolListElement ) );
	//if( location < 0x10000000 ) printf( "%p: %s:%s\n", location, path, name );
	new->path = path?strdup( path ):0;
	new->name = name?strdup( name ):0;
	new->location = location;
	new->size = size;
	new->next = scl;
	scl = new;
	return 0;
}

struct SymbolListElement * GetSymbolFromAddress( void * location )
{
	struct SymbolListElement * best = 0;
	struct SymbolListElement * walk = scl;
	for( ; walk; walk = walk->next )
	{
		if( walk->location <= location && location <= walk->location + walk->size )
		{
			best = walk;
		}
	}
	return best;
}


LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
	printf( "**CRASH**\n" );
	int stacktop;
	const char * reason;
	static char * stack_trace_buffer;
	static char * stp;
	int i;
	stack_trace_buffer = stp = malloc(16384);

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )
	struct ReasonTableType { int reason; const char * err; int recoverable; } reasontable[] = {
		{ EXCEPTION_ACCESS_VIOLATION, "Access Violation", 1 },
		{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "Bounds Exceeded", 1 },
		{ EXCEPTION_BREAKPOINT, "Breakpoint", 1 },
		{ EXCEPTION_DATATYPE_MISALIGNMENT, "Misalignmened Data", 1 },
		{ EXCEPTION_FLT_DENORMAL_OPERAND, "Denormal Opreand", 1 },
		{ EXCEPTION_FLT_DIVIDE_BY_ZERO, "Divide By Zero", 1 },
		{ EXCEPTION_FLT_INEXACT_RESULT, "Inexact Result", 1 },
		{ EXCEPTION_FLT_INVALID_OPERATION, "Invalid Operation", 1 },
		{ EXCEPTION_FLT_OVERFLOW, "Overflow", 1 },
		{ EXCEPTION_FLT_STACK_CHECK, "Stack Check", 1 },
		{ EXCEPTION_FLT_UNDERFLOW, "Float Underflow", 1 },
		{ EXCEPTION_ILLEGAL_INSTRUCTION, "Illegal Instruction", 1 },
		{ EXCEPTION_IN_PAGE_ERROR, "Page Error", 1 },
		{ EXCEPTION_INT_DIVIDE_BY_ZERO, "Divide by Zero", 1 },
		{ EXCEPTION_INT_OVERFLOW, "Overflow", 1 },
		{ EXCEPTION_INVALID_DISPOSITION, "Invalid Disposition", 1 },
		{ EXCEPTION_NONCONTINUABLE_EXCEPTION, "Noncontinuable Exception", 0 },
		{ EXCEPTION_PRIV_INSTRUCTION, "Priv Instruction", 1 },
		{ EXCEPTION_SINGLE_STEP, "Single Step", 1 },
		{ EXCEPTION_STACK_OVERFLOW, "Stack Overflow", 1 },
		{ -1, "Unknown Violation", 1 },
	};
	for( i = 0; reasontable[i].reason != -1 && reasontable[i].reason != ExceptionInfo->ExceptionRecord->ExceptionCode; i++ );
	reason = reasontable[i].err;
	
	int * place = (int*)ExceptionInfo->ContextRecord->Rsp;
	for( i = 0; i < 200; i++ )
	{
		int * pl = (int*)place[i];
		if( i == 0 )
		{
			pl = (int*)ExceptionInfo->ContextRecord->Rip;
		}
		if( !pl ) continue;
		struct SymbolListElement * sel = GetSymbolFromAddress( pl );
		if( sel )
		{
			stp += sprintf( stp, "[%p] %s:%s(+0x%02x)\n", pl, sel->path, sel->name, pl-sel->location );
		}
		else
		{
			stp += sprintf( stp, "[%16llx]\n", pl );
		}
	}
#else
	#error Unsupported arch ATM.
#endif
	
	int stop_execution = ((int (*)( const char *, const char *))tcccr)( reason, stack_trace_buffer );
	free( stack_trace_buffer );
#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )
	return (stop_execution?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_EXECUTION);
#endif
}


void InstallTCCCrashHandler( void * handler )
{
	tcccr = handler;
	int r = EnumerateSymbols( CrashSymEnumeratorCallback );
	if( r != 0 )
	{
		fprintf( stderr, "EnumerateSymbols error = %d\n", r );
	}
	SetUnhandledExceptionFilter( windows_exception_handler );
}

