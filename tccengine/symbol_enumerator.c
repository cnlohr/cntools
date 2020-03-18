#include <stdio.h>
#include "symbol_enumerator.h"

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
