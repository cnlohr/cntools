#include "tcccrash.h"

#ifdef TCCCRASH_STANDALONE
#define CNRBTREE_IMPLEMENTATION
#endif
#include <cnrbtree.h>

#include <setjmp.h>

#define CRASHDUMPBUFFER 32768

struct tcccrashcheckpoint_t
{
	jmp_buf jmpbuf;
	int did_crash;
	int can_jump;
	char * lastcrash;
} tcccrashcheckpoint;
static tcccrashcheckpoint scratchcp;

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )

#include <windows.h>
#define USE_WINDOWS
static DWORD tlskey;
#define setupkey()  tlskey = TlsAlloc();
#define deletekey() TlsFree( tlskey );
#define readkey     (tcccrashcheckpoint*)TlsGetValue( tlskey )
#define setkey(x)   TlsSetValue( tlskey, (void*)x )

LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
	int stacktop;

	//Avoid use of heap here.
	printf( "**CRASH**\n" );
	tcccrashcheckpoint * cp = readkey;
	if( !cp ) cp = &scratchcp;

	char * stp;
	int i;
	cp->did_crash = 1;
	stp = cp->lastcrash;

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
	stp += sprintf( stp, "%s\n", reasontable[i].err );
	
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
		if( stp > CRASHDUMPBUFFER - 8192 ) break; //Just in case it's waaay too long
	}

	if( cp->can_jump ) longjmp( cp->jmpbuf );

	printf( "Untracked thread crashed.\s );
	puts( cp->lastcrash );
	return EXCEPTION_EXECUTE_HANDLER;  //EXCEPTION_CONTINUE_EXECUTION
}

int CrashSymEnumeratorCallback( const char * path, const char * name, void * location, long size )
{
	struct tcccrash_syminfo * si = malloc( sizeof( struct tcccrash_syminfo ) );
	si->tag = 0;
	si->symbol = strdup( name );
	si->path = strdup( path );
	si->address = location;
	si->size = size;
	tcccrash_symset( 0, si );
	return 0;
}

static void EnumerateSymbols()
{
	int r = EnumerateSymbols( CrashSymEnumeratorCallback );
	if( r != 0 )
	{
		fprintf( stderr, "EnumerateSymbols error = %d\n", r );
	}
}

static void SetupCrashHandler()
{
	SetUnhandledExceptionFilter( windows_exception_handler );
}

#else

#include <pthread.h>
#include <signal.h>

static pthread_key_t tlskey;
#define setupkey()  (void) pthread_key_create(&tlskey, NULL)
#define deletekey() pthread_key_delete(tlskey)
#define readkey     (tcccrashcheckpoint*)pthread_getspecific(tlskey)
#define setkey(x)   pthread_setspecific(tlskey, (void*)x) 


static void EnumerateSymbols()
{
}

void sighandler(int sig, struct sigcontext ctx)
{
	int stacktop;

	//Avoid use of heap here.
	printf( "**CRASH**\n" );
	tcccrashcheckpoint * cp = readkey;
	if( !cp ) cp = &scratchcp;
	static char * stp;
	int i;
	cp->did_crash = 1;
	stp = cp->lastcrash;

#if 0
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
#endif

#if 0 //If we were a sig_action (but we're not)
	sprintf( stp, "si_signo = %d\n", si->si_signo );
	sprintf( stp, "si_code = %d\n", si->si_code );
	sprintf( stp, "si_value = %p\n", si->si_value.sival_ptr );
	sprintf( stp, "si_errno = %d\n", si->si_errno );
	sprintf( stp, "si_pid = %d\n", si->si_pid );
	sprintf( stp, "si_uid = %d\n", si->si_uid );
	sprintf( stp, "si_addr = %p\n", si->si_addr );
	tcccrash_symget( si->si_addr,  &symadd )
	sprintf( stp, "  %p: %s:%s(+0x%02x)\n", si->si_addr, symadd->symbol, symadd->path, symadd->address - si->si_addr );
#endif

	//https://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/063/6391/6391l2.html
	tcccrash_syminfo * symadd;
	tcccrash_symget( ctx.eip,  &symadd )
	sprintf( stp, "  %p: %s:%s(+0x%02x)\n", ctx.eip, symadd->symbol, symadd->path, symadd->address - ctx.eip );
	

	if( sel )
	{
		stp += sprintf( stp, "[%p] %s:%s(+0x%02x)\n", pl, sel->path, sel->name, pl-sel->location );
	}
	else
	{
		stp += sprintf( stp, "[%16llx]\n", pl );
	}



	sprintf( stp, "si_status = %d\n", si->si_status );
	sprintf( stp, "si_band = %d\n", si->si_band );

	for( i = 0; i < 200; i++ )
	{
		
	}

	if( stp > CRASHDUMPBUFFER - 8192 ) break; //Just in case it's waaay too long


	if( cp->can_jump ) longjmp( cp->jmpbuf );
	printf( "Untracked thread crashed.\s );
	puts( cp->lastcrash );
	exit( -1 );
}


static void SetupCrashHandler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sighandler;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
#ifdef SIGBUS
    sigaction(SIGBUS, &sa, NULL);
#endif
}


#endif













void tcccrash_install()
{
	setupkey();
	EnumerateSymbols();
	SetupCrashHandler();

	//For untracked threads.
	sratch->lastcrash = malloc( CRASHDUMPBUFFER );
}

void tcccrash_uninstall()
{
	deletekey();
}

int  tcccrash_checkpoint()
{
	tcccrashcheckpoint * cp = pthread_getspecific
	if( cp == 0 )
	{
		cp = malloc( sizeof( tcccrashcheckpoint ) );
		memset( cp, 0, sizeof( tcccrashcheckpoint ) );
		cp->lastcrash = malloc( CRASHDUMPBUFFER );
		setkey( cp );
	}
	cp->can_jump = 1;
	return setjmp( cp->jmpbuf );
}

char * tcccrash_getcrash()
{
	tcccrashcheckpoint * cp = pthread_getspecific
	return cp?cp->lastcrash:0;
}

void tcccrash_closethread()
{
	tcccrashcheckpoint * cp = pthread_getspecific
	if( cp )
	{
		if( cp->lastcrash ) free( cp->lastcrash );
		free( cp );
	}
}

typedef tcccrash_syminfo * simi;
typedef void * ptr;

#define delsymi( key, data ) \
	if( data ) { \
		if( data->symbol ) free( data->symbol ); \
		if( data->path ) free( data->path ); \
		free( data );
	}

#define deltree( key, data ) \
	if( data ) RBDESTROY( *data )

CNRBTREETEMPLATE( ptr, simi, RBCBPTR, delsymi ); //defines cnrbtree_ptrsimi
typedef cnrbtree_ptrsimi * ptrsimi;
CNRBTREETEMPLATE( ptr, ptrsimi, RBCBPTR, deltree ); //defines cnrbtree_ptrptrsimi

static cnrbtree_ptrptrsemi symroot;

void tcccrash_symset( void * tag, tcccrash_syminfo * symadd )
{
	ptrsimi tagptr = RBA( symroot, tag );
	if( !tagptr )
	{
		tagptr = cnrbtree_ptrsimi_create();
		RBA( symroot, tag ) = tagptr;
	}

	tcccrash_syminfo * cs = &RBA( tagptr->data, symadd->address );
	memcpy( cs, symadd, sizeof( *cs ) );
}

void tcccrash_deltag( void * tag )
{
	cnrbtree_ptrptrsimi_delete( symroot, tag );
}

int tcccrash_symget( intptr_t address, tcccrash_syminfo ** symadd )
{
	uintptr_t mindiff = MAX_INT;
	RBFOREACH( ptrptrsimi, symroot, i )
	{
		cnrbtree_ptrsimi_node * n = cnrbtree_ptrsimi_getltgt( i->data, address, 1, 0 );
		uintptr_t diff = address - n->data->address;
		if( diff < mindiff ) *symadd = n->data;
	}
}




