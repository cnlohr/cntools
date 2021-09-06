//
// Don't forget to compile with -rdynamic.
//
// Stripped down version of crash handler from cnovr.
//
// Right now, just tested in Windows.  Linux needs a little work.

#ifndef _TCC_CRASH_H
#define _TCC_CRASH_H

#include <stdint.h>
#include <setjmp.h>

//Enumerates all symbols in the currently loaded excutable.
//Don't forget to compile with -rdynamic!

//Return 0 to continue search.  1 to stop.
typedef int (*SymEnumeratorCallback)( const char * path, const char * name, void * location, long size );

int EnumerateSymbols( SymEnumeratorCallback cb );

void tcccrash_install();
void tcccrash_uninstall();

typedef struct tcccrash_syminfo_t
{
	intptr_t tag;
	char * name;
	char * path;
	intptr_t address;
	int size;
} tcccrash_syminfo;

#define NO_INTERNAL_TCC_SYM_MONITOR
#define CRASHDUMPSTACKSIZE 8192
#define CRASHDUMPBUFFER 16384
#define MAXBTDEPTH 64

// The buffer for the crash.
extern char last_crash_text[CRASHDUMPBUFFER];


#ifndef LIBTCC_H
typedef struct TCCState TCCState;
#endif

//This consumes symadd.
void tcccrash_symset( tcccrash_syminfo * symadd );
tcccrash_syminfo * tcccrash_symget( intptr_t address );
void * tcccrash_symaddr( const char * symbol );
void tcccrash_symtcc( const char * file, TCCState * tcc );

#endif

#ifdef  TCC_CRASH_IMPLEMENTATION


#include <string.h>
#include <stdio.h>

#ifndef TCCCRASH_THREAD_MALLOC
#define TCCCRASH_THREAD_MALLOC  malloc
#endif
#ifndef TCCCRASH_THREAD_FREE
#define TCCCRASH_THREAD_FREE    free
#endif

//XXX TODO: Windows support untested.
//XXX TODO: have some mechanism/flag to say "if this thread crashes, just kill it but don't exit the program
//XXX TODO: can we find a way we don't have to copy the symbol names and paths?


// The buffer for the crash.
char last_crash_text[CRASHDUMPBUFFER];

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )

#include <windows.h>
#define USE_WINDOWS


LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS * ExceptionInfo)
{
	int stacktop;

	//Avoid use of heap here.
	printf( "**CRASH**\n" );

	char * stp = last_crash_text;
	int i;

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
	
	intptr_t * place = (intptr_t*)ExceptionInfo->ContextRecord->Rsp;
	for( i = 0; i < 200; i++ )
	{
		intptr_t * pel = (intptr_t*)place[i];
		if( i == 0 )
		{
			pel = (intptr_t*)ExceptionInfo->ContextRecord->Rip;
		}
		if( !pel ) continue;
		tcccrash_syminfo * sel = tcccrash_symget( (intptr_t)pel );
		if( sel )
		{
			int delta = ((uint8_t*)pel-(uint8_t*)sel->address);
			if( delta < (sel->size + 0xffff) )
				stp += sprintf( stp, "[%p] %s : %s (+0x%02x)%s\n", (void*)pel, sel->path, sel->name, (int)delta, (delta>(sel->size+128))?" DUBIOUS":"" );
			else
				stp += sprintf( stp, "[%p] ????\n", (void*)pel );
		}
		else
		{
			stp += sprintf( stp, "[%16llx]\n", pel );
		}
		if( stp - last_crash_text > CRASHDUMPBUFFER - 1024 ) break; //Just in case it's waaay too long
	}

	printf( "Untracked thread crashed.\n" );
	puts( last_crash_text );
	return EXCEPTION_EXECUTE_HANDLER;  //EXCEPTION_CONTINUE_EXECUTION
}


static void SetupCrashHandler()
{
	SetUnhandledExceptionFilter( windows_exception_handler );
}

static void UnsetCrashHandler()
{
	SetUnhandledExceptionFilter( 0 );
}

#else

#include <pthread.h>
#include <signal.h>

#ifndef ANDROID
#include <execinfo.h>
#endif

//Backtrace code closely modeled after the code here:
// https://stackoverflow.com/questions/47609816/get-backtrace-under-tiny-c
// Just FYI you can get BP from "Here" with:
//		asm ("mov %%rbp, %0;" : "=r" (bp));

int tccbacktrace(void **buffer, int size, void * specbp) {
#ifdef ANDROID
	buffer[0] = 0;
	return 0;
#else
    extern intptr_t *__libc_stack_end;
    intptr_t **p, *bp, *frame;
	bp = specbp;
    p = (intptr_t**) bp;
    int i = 0;
    while (i < size) {
        frame = p[0];
        if (frame < bp || frame > __libc_stack_end) {
            return i;
        }
        buffer[i++] = p[1];
        p = (intptr_t**) frame;
    }
    return i;
#endif
}

uint8_t signalstack[CRASHDUMPSTACKSIZE];
void * btrace[MAXBTDEPTH];

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


	printf( "**CRASH sighandler(%s)\n", thiscrash );

	//For untracked threads.

	if( cp->in_crash_handler == 1 )
	{
		printf( "Double-crash. Cannot recover.\n" );
		exit( -1 );
	}

	cp->in_crash_handler = 1;
	cp->did_crash = sig;
	tcccrash_syminfo * sel;
	char * stp = last_crash_text;
	char * stpstart = stp;
	void ** btrace = cp->btrace;
	int i;

#ifdef __aarch64__
	printf( "FA: %p / SP: %p / PC: %p / PS: %p\n", ctx.fault_address, ctx.sp, ctx.pc, ctx.pstate );
	for( i = 0; i < 31; i++ )
	{
		intptr_t ip = ctx.regs[i];
                sel = tcccrash_symget( ip );
                ip = (intptr_t)btrace[i];
                if( sel )
                {
                        printf( "%2d [%p] %s : %s(+0x%02x)\n", i, (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip-sel->address) );
                        stp += sprintf( stp, "%2d [%p] %s : %s(+0x%02x)\n", i, (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip-sel->address) );
                }
                else
                {
                        printf( "%2d [%p]\n", i, (void*)ip );
                        stp += sprintf( stp, "%2d [%p]\n", i, (void*)ip );
                }
	}
#endif

	//https://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/063/6391/6391l2.html
	intptr_t ip;
	#ifdef __i386__
	ip = ctx.oldmask; //not eip - not sure why
	#elif __x86_64__
	ip = ctx.oldmask; //not rip - not sure why
	#else
	ip = ctx.fault_address; //Not sure what we can get here
	#endif
	printf( "    %p\n", (void*)ip );
	sel = tcccrash_symget( ip );
	if( sel )
	{
		stp += sprintf( stp, "[%p] %s : %s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip - sel->address) );
		printf( "    %s(+0x%02x)**\n", sel->name, (int)(intptr_t)(ip - sel->address) );
	}
	else
	{
		stp += sprintf( stp, "[%p]\n", (void*)ip );
		printf( "    %p**\n", (void*)ip );
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
	puts( stpstart );
	//int btl = backtrace( (void**)btrace, MAXBTDEPTH );
#ifdef __aarch64__
	void * rsp = (void*)ctx.sp;
#else
	void * rsp = (void*)ctx.rsp;
#endif

	if( rsp < (void*)0x40000 )
	{
		//rsp broken.  Try RBP
#ifdef __aarch64__
		rsp = (void*)ctx.regs[1];
#else
		rsp = (void*)ctx.rbp;
#endif
	}
	if( rsp < (void*)0x400000 )
	{
		printf( "No backtrace RSP.  Can't take backtrace.\n" );
		stp += sprintf( stp, "No backtrace RSP.  Can't take backtrace.\n" );
		exit( -1 );
		goto  finishup;
	}
	printf( "Backtrace RSP: %p\n", rsp ); 
	int btl = tccbacktrace((void**)btrace, MAXBTDEPTH, rsp );
	stp += sprintf( stp, "==========================================%d [%p]\n", btl, rsp );
	for( i = 0; i < btl; i++ )
	{
		sel = tcccrash_symget( (intptr_t)btrace[i] );
		ip = (intptr_t)btrace[i];
		if( sel )
		{
			printf( "[%p] %s : %s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip-sel->address) );
			stp += sprintf( stp, "[%p] %s : %s(+0x%02x)\n", (void*)ip, sel->path, sel->name, (int)(intptr_t)(ip-sel->address) );
		}
		else
		{
			printf( "[%p]\n", (void*)ip );
			stp += sprintf( stp, "[%p]\n", (void*)ip );
		}

		if( stp - stpstart > CRASHDUMPBUFFER - 1024 ) break; //Just in case it's waaay too long
	}
	printf( "Crash handler complete. Continuing execution\n" );
finishup:
	cp->in_crash_handler = 0;
	if( cp->can_jump ) siglongjmp( cp->jmpbuf, sig );
	printf( "Untracked thread crashed.\n" );
	puts( cp->lastcrash );
	exit( -1 );
}


static void UnsetCrashHandler()
{
	sigaction(SIGSEGV, 0, NULL);
	sigaction(SIGABRT, 0, NULL);
	sigaction(SIGFPE, 0, NULL);
	sigaction(SIGILL, 0, NULL);
	sigaction(SIGALRM, 0, NULL); //Maybe have this to watchdog tcc tasks?
#ifdef SIGBUS
	sigaction(SIGBUS, 0, NULL);
#endif
}

static void SetupCrashHandler()
{
	void * v;
	//This forces a library-load for the backtrace() function to stop it from
	//malloc'ing in runtime.
	//backtrace( &v, 1); 

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
	printf( "Sighandler installed.\n" );
#ifdef SIGBUS
	sigaction(SIGBUS, &sa, NULL);
#endif
}


#endif


static int TCCCrashSymEnumeratorCallback( const char * path, const char * name, void * location, long size )
{
	//printf( "%s / %s / %p / %d\n", path, name, location, size );
	if( !name || strlen(name) < 1 ) return 0;
	tcccrash_syminfo * sn = malloc( sizeof( tcccrash_syminfo ) );
	memset( sn, 0, sizeof( *sn ) );
	sn->name = strdup( name );
	sn->path = strdup( path );
	sn->tag = 0;
	sn->address = (intptr_t)location;
	sn->size = size;
	tcccrash_symset( sn );
	return 0;
}


void tcccrash_install()
{
	printf( "Running tcccrash_install()\n" );
	SetupCrashHandler();
	EnumerateSymbols( TCCCrashSymEnumeratorCallback );
}

void tcccrash_uninstall()
{
	UnsetCrashHandler();
}


tcccrash_syminfo ** known_symbols;
int num_known_symbols;

//This consumes symadd, it will handle freeing it later.
void tcccrash_symset( tcccrash_syminfo * symadd )
{
	known_symbols = realloc( known_symbols, sizeof( tcccrash_syminfo * ) * num_known_symbols + 1 );
	known_symbols[num_known_symbols++] = symadd;	
}

tcccrash_syminfo * tcccrash_symget( intptr_t address )
{
	int i;
	tcccrash_syminfo * closest = 0;
	intptr_t mindiff = 0x7fffffff;
	for( i = 0; i < num_known_symbols; i++ )
	{
		tcccrash_syminfo * ths = known_symbols[i];
		intptr_t diff = address - ths->address;
		if( diff > 0 && diff < mindiff )
		{
			mindiff = diff;
			closest = ths;
		}
	}
	return closest;
}

void * tcccrash_symaddr( const char * symbol )
{
	int i;
	for( i = 0; i < num_known_symbols; i++ )
	{
		tcccrash_syminfo * ths = known_symbols[i];
		if( strcmp( ths->name, symbol ) == 0 )
			return (void*)ths->address;
	}
	return 0;
}

static tcccrash_syminfo * dupsym( const char * name, const char * path )
{
	tcccrash_syminfo * ret = malloc( sizeof( tcccrash_syminfo ) );
	memset( ret, 0, sizeof( *ret ) );
	//printf( "DUPSYM: %s %s\n", name, path );
	ret->name = strdup( name );
	ret->path = strdup( path );
	return ret;
}

#ifndef NO_INTERNAL_TCC_SYM_MONITOR

#include <tinycc/tcc.h>

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
	OGLockMutex( symroot_mutex );
	cnrbtree_ptrstrsimi_node * nrt = cnrbtree_ptrstrsimi_get( symroot_name, tag  );
	if( !nrt ) goto fail;
	cnrbtree_strsimi_node * n = cnrbtree_strsimi_get( nrt->data, (char*)symbol );
	if( !n || !n->data ) goto fail;
	OGUnlockMutex( symroot_mutex );
	return (void*)n->data->address;
fail:
	OGUnlockMutex( symroot_mutex );
	return 0;
}


#endif



////////////////////////////////////////////////////////////////////////////////
// Symbol Enumerator.


#include <stdio.h>

#if defined( WIN32 ) || defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( _WIN32 )

#include <windows.h>

#ifdef __TINYC__


typedef struct _SYMBOL_INFO {
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    CHAR        Name[1];          // Name of symbol
} SYMBOL_INFO, *PSYMBOL_INFO;

#define _In_
#define _In_opt_
#define IMAGEAPI __declspec( dllimport )
typedef BOOL
(CALLBACK *PSYM_ENUMERATESYMBOLS_CALLBACK)(
    _In_ PSYMBOL_INFO pSymInfo,
    _In_ ULONG SymbolSize,
    _In_opt_ PVOID UserContext
    );

BOOL
IMAGEAPI
SymInitialize(
    _In_ HANDLE hProcess,
    _In_opt_ PCSTR UserSearchPath,
    _In_ BOOL fInvadeProcess
    );

BOOL
IMAGEAPI
SymEnumSymbols(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCSTR Mask,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    );

BOOL
IMAGEAPI
SymCleanup(
    _In_ HANDLE hProcess
    );

#else
#include <dbghelp.h>
#endif

BOOL CALLBACK mycb(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
	SymEnumeratorCallback cb = (SymEnumeratorCallback)UserContext;
	return !cb("", &pSymInfo->Name[0], (void *)pSymInfo->Address, (long)pSymInfo->Size);
  }
  
int EnumerateSymbols( SymEnumeratorCallback cb )
{
	HANDLE proc = GetCurrentProcess();
	if( !SymInitialize( proc, 0, 1 ) ) return -1;
	if( !SymEnumSymbols( proc, 0, "*!*", &mycb, (void*)cb ) )
	{
		fprintf(stderr, "SymEnumSymbols returned %d\n", GetLastError());
		SymCleanup(proc);
		return -2;
	}
	SymCleanup(proc);
	return 0;
}

#else
	

#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#ifndef __GNUC__
#define __int128_t long // not actually used by us.
#endif

#include <link.h>
#include <elf.h>

#define UINTS_PER_WORD (__WORDSIZE / (CHAR_BIT * sizeof (unsigned int)))

#ifndef ANDROID
    struct dl_phdr_info {
        ElfW(Addr)        dlpi_addr;  /* Base address of object */
        const char       *dlpi_name;  /* (Null-terminated) name of
                                         object */
        const ElfW(Phdr) *dlpi_phdr;  /* Pointer to array of
                                         ELF program headers
                                         for this object */
        ElfW(Half)        dlpi_phnum; /* # of items in dlpi_phdr */
    };
	void dl_iterate_phdr( void*, void*);
#endif


static ElfW(Word) gnu_hashtab_symbol_count(const unsigned int *const table)
{
    const unsigned int *const bucket = table + 4 + table[2] * (unsigned int)(UINTS_PER_WORD);
    unsigned int              b = table[0];
    unsigned int              max = 0U;

    while (b-->0U)
        if (bucket[b] > max)
            max = bucket[b];

    return (ElfW(Word))max;
}

static void *dynamic_pointer(const ElfW(Addr) addr, const ElfW(Addr) base, const ElfW(Phdr) *const header,
							 const ElfW(Half) headers) {
	if (addr) {
        ElfW(Half) h;

        for (h = 0; h < headers; h++)
            if (header[h].p_type == PT_LOAD)
                if (addr >= base + header[h].p_vaddr &&
                    addr <  base + header[h].p_vaddr + header[h].p_memsz)
                    return (void *)addr;
    }

    return NULL;
}

//Mostly based off of http://stackoverflow.com/questions/29903049/get-names-and-addresses-of-exported-functions-from-in-linux
static int callback(struct dl_phdr_info *info,
               size_t size, void *data)
{
	SymEnumeratorCallback cb = (SymEnumeratorCallback)data;
	const ElfW(Addr)                 base = info->dlpi_addr;
	const ElfW(Phdr) *const          header = info->dlpi_phdr;
	const ElfW(Half)                 headers = info->dlpi_phnum;
	const char                      *libpath, *libname;
	ElfW(Half)                       h;

	if (info->dlpi_name && info->dlpi_name[0])
		libpath = info->dlpi_name;
	else
		libpath = "???";

	libname = strrchr(libpath, '/');

	if (libname && libname[0] == '/' && libname[1])
		libname++;
	else
		libname = libpath;

	for (h = 0; h < headers; h++)
	{
		if (header[h].p_type == PT_DYNAMIC)
		{
			const ElfW(Dyn)  *entry = (const ElfW(Dyn) *)(base + header[h].p_vaddr);
			const ElfW(Word) *hashtab;
			const ElfW(Sym)  *symtab = NULL;
			const char       *strtab = NULL;
			ElfW(Word)        symbol_count = 0;
			for (; entry->d_tag != DT_NULL; entry++)
			{
				switch (entry->d_tag)
				{
				case DT_HASH:
					hashtab = dynamic_pointer(entry->d_un.d_ptr, base, header, headers);
					if (hashtab)
						symbol_count = hashtab[1];
					break;
				case DT_GNU_HASH:
					hashtab = dynamic_pointer(entry->d_un.d_ptr, base, header, headers);
					if (hashtab)
					{
						ElfW(Word) count = gnu_hashtab_symbol_count(hashtab);
						if (count > symbol_count)
							symbol_count = count;
					}
					break;
				case DT_STRTAB:
					strtab = dynamic_pointer(entry->d_un.d_ptr, base, header, headers);
					break;
				case DT_SYMTAB:
					symtab = dynamic_pointer(entry->d_un.d_ptr, base, header, headers);
					break;
				}
			}

			if (symtab && strtab && symbol_count > 0) {
				ElfW(Word)  s;

				for (s = 0; s < symbol_count; s++) {
					const char *name;
					void *const ptr = dynamic_pointer(base + symtab[s].st_value, base, header, headers);
					int         result;

					if (!ptr)
						continue;

					if (symtab[s].st_name)
						name = strtab + symtab[s].st_name;
					else
						name = "";
					result = cb( libpath, name, ptr, symtab[s].st_size );
					if( result ) return result; //Bail early.
				}
			}
		}
	}
	return 0;
}





int EnumerateSymbols( SymEnumeratorCallback cb )
{
	dl_iterate_phdr( callback, cb );
	return 0;
}




#endif


#endif


