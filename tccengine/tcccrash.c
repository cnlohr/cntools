#include "tcccrash.h"
#include <stdio.h>

//XXX TODO: Windows support untested.
//XXX TODO: have some mechanism/flag to say "if this thread crashes, just kill it but don't exit the program
//XXX TODO: can we find a way we don't have to copy the symbol names and paths?

#ifdef TCCCRASH_STANDALONE
#define CNRBTREE_IMPLEMENTATION
#endif
#include <cnrbtree.h>


#define CRASHDUMPBUFFER 8192
#define MAXBTDEPTH 64

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
			stp += sprintf( stp, "[%p] %s:%s(+0x%02x)\n", (void*)pl, sel->path, sel->name, (int)(pl-sel->size) );
		}
		else
		{
			stp += sprintf( stp, "[%16llx]\n", pl );
		}
		if( stp - stpstart > CRASHDUMPBUFFER - 1024 ) break; //Just in case it's waaay too long
	}

	if( cp->can_jump ) longjmp( cp->jmpbuf, -1 );

	printf( "Untracked thread crashed.\s" );
	puts( cp->lastcrash );
	return EXCEPTION_EXECUTE_HANDLER;  //EXCEPTION_CONTINUE_EXECUTION
}

int CrashSymEnumeratorCallback( const char * path, const char * name, void * location, long size )
{
	struct tcccrash_syminfo * si = malloc( sizeof( struct tcccrash_syminfo ) );
	si->tag = 0;
	si->name = strdup( name );
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
#include <execinfo.h>

static pthread_key_t tlskey;
#define setupkey()  (void) pthread_key_create(&tlskey, NULL)
#define deletekey() pthread_key_delete(tlskey)
#define readkey     (tcccrashcheckpoint*)pthread_getspecific(tlskey)
#define setkey(x)   pthread_setspecific(tlskey, (void*)x) 


static void EnumerateSymbols()
{
#if 0
typedef struct tcccrash_syminfo_t
{
	void  * tag;
	char * name;
	char * path;
	intptr_t address;
	int size;
} tcccrash_syminfo;
#endif
}


int tccbacktrace(void **buffer, int size, void * specbp) {
    extern uint64_t *__libc_stack_end;
    uint64_t **p, *bp, *frame;
    if( !specbp )
	{
		asm ("mov %%rbp, %0;" : "=r" (bp));
	}
	else
	{
		bp = specbp;
	}		
    p = (uint64_t**) bp;
    int i = 0;
    while (i < size) {
        frame = p[0];
        if (frame < bp || frame > __libc_stack_end) {
            return i;
        }
        buffer[i++] = p[1];

        p = (uint64_t**) frame;
    }
    return i;
}

void sighandler(int sig,  siginfo_t *info, struct sigcontext ctx)
{
	//Avoid use of heap here, minimize stack use, too.
	printf( "**CRASH sighandler(%d)**\n", sig );
	tcccrashcheckpoint * cp = readkey;
	if( !cp ) cp = &scratchcp;
	cp->did_crash = sig;
	tcccrash_syminfo * sel;
	char * stp = cp->lastcrash;
	char * stpstart = stp;
	intptr_t * btrace = cp->btrace;
	int i;


#if 0 //If we were a sig_action (but we're not)
	sprintf( stp, "si_signo = %d\n", si->si_signo );
	sprintf( stp, "si_code = %d\n", si->si_code );
	sprintf( stp, "si_value = %p\n", si->si_value.sival_ptr );
	sprintf( stp, "si_errno = %d\n", si->si_errno );
	sprintf( stp, "si_pid = %d\n", si->si_pid );
	sprintf( stp, "si_uid = %d\n", si->si_uid );
	sprintf( stp, "si_addr = %p\n", si->si_addr );
	tcccrash_symget( si->si_addr,  &symadd )
	sprintf( stp, "  %p: %s:%s(+0x%02x)\n", si->si_addr, symadd->name, symadd->path, symadd->address - si->si_addr );
#endif

	//https://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/063/6391/6391l2.html
//	printf( "CTX: %p\n", ctx.rbp );
	intptr_t ip;
	#ifdef __i386__
	ip = ctx.oldmask; //not eip - not sure why
	#else
	ip = ctx.oldmask; //not rip - not sure why
	#endif

	sel = tcccrash_symget( ip );
	if( sel )
	{
		stp += sprintf( stp, "EIP: %p: %s:%s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(ip - sel->address) );
		puts( stp );
	}
	else
	{
		stp += sprintf( stp, "[%p]\n", (void*)ip );
	}

	stp += sprintf( stp, "==========================================\n" );

	//int btl = backtrace( (void**)btrace, MAXBTDEPTH );
	int btl = tccbacktrace((void**)btrace, MAXBTDEPTH, ctx.rsp );
	for( i = 0; i < btl; i++ )
	{
		sel = tcccrash_symget( btrace[i] );
		ip = btrace[i];
		if( sel )
		{
			stp += sprintf( stp, "[%p] %s:%s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(ip-sel->address) );
		}
		else
		{
			stp += sprintf( stp, "[%p]\n", (void*)ip );
		}

		if( stp - stpstart > CRASHDUMPBUFFER - 1024 ) break; //Just in case it's waaay too long
	}
	//sprintf( stp, "si_status = %d\n", si->si_status );
	//sprintf( stp, "si_band = %d\n", si->si_band );

	printf( "%s", cp->lastcrash );
	printf( "Returning %d\n", cp->can_jump );
	if( cp->can_jump ) siglongjmp( cp->jmpbuf, sig );
	printf( "Untracked thread crashed.\n" );
	puts( cp->lastcrash );
	exit( -1 );
}


static void SetupCrashHandler()
{
	void * v;
	//This forces a library-load for the backtrace() function to stop it from
	//malloc'ing in runtime.
	backtrace( &v, 1); 

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = (void*)sighandler;//void sighandler(int sig, struct sigcontext ctx)
    sa.sa_flags   = SA_RESTART | SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL); //Maybe have this to watchdog tcc tasks?
#ifdef SIGBUS
    sigaction(SIGBUS, &sa, NULL);
#endif
}


#endif











static void setup_symbols();


void tcccrash_install()
{
	setupkey();
	EnumerateSymbols();
	SetupCrashHandler();
	setup_symbols();

	//For untracked threads.
	scratchcp.lastcrash = malloc( CRASHDUMPBUFFER );
	scratchcp.btrace = malloc( sizeof(void*) * MAXBTDEPTH );
}

void tcccrash_uninstall()
{
	deletekey();
}
char * tcccrash_getcrash()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)readkey;
	return cp?cp->lastcrash:0;
}

void tcccrash_closethread()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)readkey;
	if( cp )
	{
		if( cp->lastcrash ) free( cp->lastcrash );
		free( cp );
	}
}

tcccrashcheckpoint * tcccrash_getcheckpoint()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)readkey;
	if( cp == 0 )
	{
		cp = malloc( sizeof( tcccrashcheckpoint ) );
		memset( cp, 0, sizeof( tcccrashcheckpoint ) );
		cp->lastcrash = malloc( CRASHDUMPBUFFER );
		cp->btrace = malloc( sizeof(void*) * MAXBTDEPTH );
		cp->can_jump = 1;
		setkey( cp );
	}
	return cp;
}


typedef tcccrash_syminfo * simi;
typedef void * ptr;

#define delsymi( key, data ) \
	if( data ) { \
		if( data->name ) free( data->name ); \
		if( data->path ) free( data->path ); \
		free( data ); \
	}

#define deltree( key, data ) \
	if( data ) RBDESTROY( data )

#define RBsptrcpy(x,y,z) { x = y; z = 0; }


CNRBTREETEMPLATE( ptr, simi, RBptrcmp, RBsptrcpy, delsymi ); //defines cnrbtree_ptrsimi
typedef cnrbtree_ptrsimi * ptrsimi;
CNRBTREETEMPLATE( ptr, ptrsimi, RBptrcmp, RBsptrcpy, deltree ); //defines cnrbtree_ptrptrsimi

static cnrbtree_ptrptrsimi * symroot;

static void setup_symbols()
{
	symroot = cnrbtree_ptrptrsimi_create();
}

void tcccrash_symset( void * tag, tcccrash_syminfo * symadd )
{
	ptrsimi tagptr = RBA( symroot, tag );
	if( !tagptr )
	{
		tagptr = cnrbtree_ptrsimi_create();
		RBA( symroot, tag ) = tagptr;
	}
	tcccrash_syminfo * cs = RBA( tagptr, symadd->address );
	if( !cs ) cs = malloc( sizeof( *cs ) );
	memcpy( cs, symadd, sizeof( *cs ) );
	cs->tag = tag;
	RBA( tagptr, symadd->address ) = cs;
}

void tcccrash_deltag( void * tag )
{
	cnrbtree_ptrptrsimi_delete( symroot, tag );
}

tcccrash_syminfo * tcccrash_symget( intptr_t address )
{
	tcccrash_syminfo * ret = 0;
	uintptr_t mindiff = -1;
	RBFOREACH( ptrptrsimi, symroot, i )
	{
		cnrbtree_ptrsimi_node * n = cnrbtree_ptrsimi_get2( i->data, (void*)address, 1 );
		if( n->data->address > address )
			n = (void*)cnrbtree_generic_prev( (void*)n );
		if( !n ) continue;
		uintptr_t diff = address - n->data->address;
		if( diff < mindiff && diff < n->data->size + 16 )
		{
			ret = n->data;
			mindiff = diff;
		}
	}
	return ret;
}




