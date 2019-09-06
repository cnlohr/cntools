#include "tcccrash.h"
#include <os_generic.h>
#include <stdio.h>

// Don't forget to compile with -rdynamic.
//

//XXX TODO: Windows support untested.
//XXX TODO: have some mechanism/flag to say "if this thread crashes, just kill it but don't exit the program
//XXX TODO: can we find a way we don't have to copy the symbol names and paths?

#ifdef TCCCRASH_STANDALONE
#define CNRBTREE_IMPLEMENTATION
#endif
#include <cnrbtree.h>
#include "symbol_enumerator.h"

#define CRASHDUMPSTACKSIZE 8192
#define CRASHDUMPBUFFER 8192
#define MAXBTDEPTH 64

static og_tls_t threadcheck;

static tcccrashcheckpoint scratchcp;

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )

#include <windows.h>
#define USE_WINDOWS


LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
	int stacktop;

	//Avoid use of heap here.
	printf( "**CRASH**\n" );
	tcccrashcheckpoint * cp = OGGetTLS( threadcheck );
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
			stp += sprintf( stp, "[%p] %s : %s(+0x%02x)\n", (void*)pl, sel->path, sel->name, (int)(pl-sel->size) );
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


static void SetupCrashHandler()
{
	SetUnhandledExceptionFilter( windows_exception_handler );
}


#else

#include <pthread.h>
#include <signal.h>
#include <execinfo.h>

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
	static const char * thiscrash;
	thiscrash = "unknown";
	switch( sig )
	{
		case SIGSEGV: thiscrash = "SIGSEGV"; break;
		case SIGABRT: thiscrash = "SIGABRT"; break;
		case SIGFPE: thiscrash = "SIGFPE"; break;
		case SIGILL: thiscrash = "SIGILL"; break;
		case SIGALRM: thiscrash = "SIGALRM"; break;
#ifdef SIGBUS
		case SIGBUS: thiscrash = "SIGBUS"; break;
#endif
	}


	printf( "**CRASH sighandler(%s) in ", thiscrash );
	tcccrashcheckpoint * cp = OGGetTLS( threadcheck );
	if( !cp ) cp = &scratchcp;
	cp->did_crash = sig;
	tcccrash_syminfo * sel;
	char * stp = cp->lastcrash;
	char * stpstart = stp;
	void ** btrace = cp->btrace;
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
		stp += sprintf( stp, "EIP: %p: %s : %s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip - sel->address) );
		printf( "%s**\n", sel->name );
	}
	else
	{
		stp += sprintf( stp, "[%p]\n", (void*)ip );
		printf( "%p**\n", (void*)ip );
	}


#if 0
# ifdef __i386__
	stp += sprintf( stp, "edi: %08x esi: %08x ebp:    %08x esp: %08x ebx: %08x\n", ctx.edi, ctx.esi, ctx.ebp, ctx.esp, ctx.ebx );
	stp += sprintf( stp, "edx: %08x ecx: %08x trapno: %08x err: %08x eip: %08x\n", ctx.edx, ctx.ecx, ctx.trapno, ctx.err, ctx.eip );
	stp += sprintf( stp, "esp_at_signal: %08x cr2:    %08x\n", ctx.esp_at_signal, ctx.cr2 );
#else
	stp += sprintf( stp, "r8  %016lx r9  %016lx r10 %016lx\n", ctx.r8, ctx.r9, ctx.r10 );
	stp += sprintf( stp, "r11 %016lx r12 %016lx r13 %016lx\n", ctx.r11, ctx.r12, ctx.r13 );
	stp += sprintf( stp, "r14 %016lx r15 %016lx\n", ctx.r14, ctx.r15 );
	stp += sprintf( stp, "rdi %016lx rsi %016lx rbp %016lx\n", ctx.rdi, ctx.rsi, ctx.rbp );
	stp += sprintf( stp, "rbx %016lx rdx %016lx rax %016lx\n", ctx.rbx, ctx.rdx, ctx.rax );
	stp += sprintf( stp, "rcx %016lx rsp %016lx rip %016lx\n", ctx.rcx, ctx.rsp, ctx.rip );
	stp += sprintf( stp, "eflags %016lx err %016lx cr2 %016lx\n", ctx.eflags, ctx.err, ctx.cr2 );
	stp += sprintf( stp, "trapno %016lx oldmask %016lx\n", ctx.trapno, ctx.oldmask );
#endif
#endif
	stp += sprintf( stp, "==========================================\n" );

	//int btl = backtrace( (void**)btrace, MAXBTDEPTH );
	int btl = tccbacktrace((void**)btrace, MAXBTDEPTH, (void*)ctx.rsp );
	for( i = 0; i < btl; i++ )
	{
		sel = tcccrash_symget( (intptr_t)btrace[i] );
		ip = (intptr_t)btrace[i];
		if( sel )
		{
			stp += sprintf( stp, "[%p] %s : %s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip-sel->address) );
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

	stack_t ss;
	ss.ss_sp = scratchcp.signalstack;
	ss.ss_flags = 0;
	ss.ss_size = CRASHDUMPSTACKSIZE;
	sigaltstack( &ss, 0);


	sa.sa_flags   = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
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

static int TCCCrashSymEnumeratorCallback( const char * path, const char * name, void * location, long size )
{
	tcccrash_syminfo * sn = malloc( sizeof( tcccrash_syminfo ) );
	memset( sn, 0, sizeof( *sn ) );
	sn->name = strdup( name );
	sn->path = strdup( path );
	sn->tag = 0;
	sn->address = (intptr_t)location;
	sn->size = size;
	tcccrash_symset( 0, sn );
	return 0;
}


void tcccrash_install()
{
	scratchcp.signalstack = malloc( CRASHDUMPSTACKSIZE );
	threadcheck = OGCreateTLS();
	SetupCrashHandler();
	setup_symbols();
	EnumerateSymbols( TCCCrashSymEnumeratorCallback );

	//For untracked threads.
	scratchcp.lastcrash = malloc( CRASHDUMPBUFFER );
	scratchcp.btrace = malloc( sizeof(void*) * MAXBTDEPTH );
}

void tcccrash_uninstall()
{
	OGDeleteTLS( threadcheck );
}
char * tcccrash_getcrash()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)OGGetTLS( threadcheck );
	return cp?cp->lastcrash:0;
}

void tcccrash_closethread()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)OGGetTLS( threadcheck );
	if( cp )
	{
		if( cp->lastcrash ) free( cp->lastcrash );
		free( cp );
	}
}

tcccrashcheckpoint * tcccrash_getcheckpoint()
{
	tcccrashcheckpoint * cp = (tcccrashcheckpoint *)OGGetTLS( threadcheck );
	if( cp == 0 )
	{
		cp = malloc( sizeof( tcccrashcheckpoint ) );
		memset( cp, 0, sizeof( tcccrashcheckpoint ) );
		cp->lastcrash = malloc( CRASHDUMPBUFFER );
		cp->btrace = malloc( sizeof(void*) * MAXBTDEPTH );
		cp->can_jump = 1;
		OGSetTLS( threadcheck, cp );
	}
	return cp;
}


typedef tcccrash_syminfo * simi;
typedef intptr_t ptr;

#define delsymi( key, data ) \
	if( data ) { \
		if( data->name ) free( data->name ); \
		if( data->path ) free( data->path ); \
		free( data ); \
	}

#define deltree( key, data ) \
	if( data ) RBDESTROY( data )

#define RBsptrcpy(x,y,z) { x = y; z = 0; }

typedef char * str;
CNRBTREETEMPLATE( ptr, simi, RBptrcmp, RBsptrcpy, delsymi ); //defines cnrbtree_ptrsimi
CNRBTREETEMPLATE( str, simi, RBstrcmp, RBstrcpy, RBstrdel ); //defines cnrbtree_ptrptrsimi
typedef cnrbtree_ptrsimi * ptrsimi;
typedef cnrbtree_strsimi * strsimi;
CNRBTREETEMPLATE( ptr, strsimi, RBptrcmp, RBsptrcpy, deltree ); //defines cnrbtree_ptrstrsimi
CNRBTREETEMPLATE( ptr, ptrsimi, RBptrcmp, RBsptrcpy, deltree ); //defines cnrbtree_ptrptrsimi

static cnrbtree_ptrptrsimi * symroot;
static cnrbtree_ptrstrsimi * symroot_name;

static void setup_symbols()
{
	symroot = cnrbtree_ptrptrsimi_create();
	symroot_name = cnrbtree_ptrstrsimi_create();
}

void tcccrash_symset( intptr_t tag, tcccrash_syminfo * symadd )
{
	ptrsimi tagptr = RBA( symroot, tag );
	strsimi tagptr_name = RBA( symroot_name, tag );
	if( !tagptr )
	{
		tagptr = cnrbtree_ptrsimi_create();
		RBA( symroot, tag ) = tagptr;
		tagptr_name = cnrbtree_strsimi_create();
		RBA( symroot_name, tag ) = tagptr_name;
	}
	tcccrash_syminfo * cs = RBA( tagptr, symadd->address );
	if( !cs ) cs = malloc( sizeof( *cs ) );
	memcpy( cs, symadd, sizeof( *cs ) );
	cs->tag = tag;
	RBA( tagptr, symadd->address ) = cs;
	RBA( tagptr_name, symadd->name ) = cs;
}

void tcccrash_deltag( intptr_t tag )
{
	cnrbtree_ptrptrsimi_delete( symroot, tag );
	cnrbtree_ptrstrsimi_delete( symroot_name, tag );
}

tcccrash_syminfo * tcccrash_symget( intptr_t address )
{
	tcccrash_syminfo * ret = 0;
	intptr_t mindiff = 0;
	RBFOREACH( ptrptrsimi, symroot, i )
	{
		cnrbtree_ptrsimi_node * n = cnrbtree_ptrsimi_get2( i->data, address, 1 );
		if( n->data->address > address )
			n = (void*)cnrbtree_generic_prev( (void*)n );
		if( !n ) continue;
		intptr_t diff = address - n->data->address;
		if( mindiff == 0 || ( diff < mindiff && diff < n->data->size + 16 ) )
		{
			ret = n->data;
			mindiff = diff;
		}
	}
	return ret;
}

static tcccrash_syminfo * dupsym( const char * name, const char * path )
{
	tcccrash_syminfo * ret = malloc( sizeof( tcccrash_syminfo ) );
	memset( ret, 0, sizeof( *ret ) );
	ret->name = strdup( name );
	ret->path = strdup( path );
	return ret;
}

#include <tcc.h>

void tcccrash_symtcc( const char * file, TCCState * state )
{
	Section * symtab = state->symtab;
	Section * hs = symtab->hash;
	int nbuckets = ((int *)hs->data)[1];
	int i;
	for( i = 0; i < nbuckets; i++ )
	{
		int sym_index = i;
		ElfW(Sym) * sym = &((ElfW(Sym) *)symtab->data)[sym_index];
		const char * name1 = (char *) symtab->link->data + sym->st_name;
		tcccrash_syminfo * sn = dupsym( name1, file );
		sn->tag = (intptr_t) state;
		sn->address = sym->st_value;
		sn->size = sym->st_size;
		tcccrash_symset( (intptr_t)state, sn );
	}
}

void * tcccrash_symaddr( intptr_t tag, const char * symbol )
{
	cnrbtree_ptrstrsimi_node * nrt = cnrbtree_ptrstrsimi_get( symroot_name, tag  );
	if( !nrt ) return 0;
	cnrbtree_strsimi_node * n = cnrbtree_strsimi_get( nrt->data, (char*)symbol );
	if( !n || !n->data ) return 0;
	return (void*)n->data->address;
}



