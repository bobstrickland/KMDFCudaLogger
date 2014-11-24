#ifndef __PageTableManipulation_h__ 
#define __PageTableManipulation_h__ 

#include <ntdef.h>

typedef struct _PTE
{
	ULONG Valid : 1;
	ULONG Writable : 1;
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	// Protection
		ULONG Accessed : 1;
		ULONG Dirty : 1;
		ULONG LargePage : 1;
		ULONG Global : 1;
		ULONG CopyOnWrite : 1;
	ULONG Prototype : 1;
	ULONG Transition : 1;
	ULONG PageFrameNumber : 20;
} PTE, *PPTE;

#define PTE_SIZE 8
#define PAGE_SIZE 0x1000

#define	PROCESS_PAGE_DIRECTORY_BASE		0xC0600000 
#define PROCESS_PAGE_TABLE_BASE			0xC0000000 

PPTE GetPteAddress(PVOID VirtualAddress);
ULONG GetPageDirectoryBaseRegister();
PHYSICAL_ADDRESS GetPDBRPhysicalAddress();
ULONG GetPhysAddress(void * virtualaddr);
ULONG GetPhysAddressPhysically(void * virtualaddr);
#endif
