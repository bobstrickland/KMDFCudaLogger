#ifndef __PageTableManipulation_h__ 
#define __PageTableManipulation_h__ 

#include <ntdef.h>
#include <ntddk.h>
#include <wdf.h>
#include <mi.h>


typedef struct _PTE
{
	union {
		ULONG rawValue;
		struct {
			ULONG Valid : 1;
			ULONG Writable : 1;
			ULONG Owner : 1;
			ULONG WriteThrough : 1;
			ULONG CacheDisable : 1;
			ULONG Accessed : 1;
			ULONG Dirty : 1;
			ULONG PageAttributeTable : 1;
			ULONG Global : 1;
			ULONG CopyOnWrite : 1;
			ULONG Prototype : 1;
			ULONG Transition : 1;
#ifndef _WIN64
			ULONG PageFrameNumber : 20;
#else
			ULONG PageFrameNumber : 28;
			ULONG Reserved : 12;
			ULONG SoftwareWorkingSetIndex : 11;
			ULONG NoExecute : 1;
#endif
		} DUMMYSTRUCTNAME;
	};
} PTE, *PPTE;

typedef struct _PFN {

	 UINT32 flink;
	 UINT32 blink;
	 UINT32 pteaddress;
	 UINT16 reference_count;
	 UINT8  page_state;
	 UINT8  flags;
	 UINT32 restore_pte;
	 UINT32 containing_page;
} PFN, *PPFN;



typedef struct _PDE
{
	union {
		ULONG rawValue;
		struct {
			ULONG Valid : 1;
			ULONG Writable : 1;
			ULONG Owner : 1;
			ULONG WriteThrough : 1;
			ULONG CacheDisable : 1;
			ULONG Accessed : 1;
			ULONG Available : 1;
			ULONG LargePage : 1;
			ULONG Global : 1;
			ULONG CopyOnWrite : 1;
			ULONG Prototype : 1;
			ULONG Transition : 1;
#ifndef _WIN64
			ULONG PageFrameNumber : 20;
#else
			ULONG PageFrameNumber : 28;
			ULONG Reserved : 12;
			ULONG SoftwareWorkingSetIndex : 11;
			ULONG NoExecute : 1;
#endif
		} DUMMYSTRUCTNAME;
	};
} PDE, *PPDE;

#define X32_PTE_SIZE					4
#define X32_PDE_SIZE					4
#define X32_PFN_SIZE					0x18
#define	X32_PROCESS_PAGE_DIRECTORY_BASE	0xC0300000 
#define X32_PROCESS_PAGE_TABLE_BASE		0xC0000000 
#define X32_PFN_DATABASE_BASE           0x83C00000
#define PAE_PTE_SIZE					8
#define PAE_PDE_SIZE					8
#define PAE_PFN_SIZE					0x1C 
#define	PAE_PROCESS_PAGE_DIRECTORY_BASE 0xC0600000 
#define PAE_PROCESS_PAGE_TABLE_BASE		0xC0000000 
#define PAE_PFN_DATABASE_BASE           0x83A00000

ULONG getPdeSize();
ULONG getPteSize();
ULONG getPageDirectoryBase();
ULONG getPageTableBase();
ULONG64 getPfnDatabaseBase();
ULONG64 getPfnSize();
BOOLEAN IsLargePage(PVOID virtualAddress);
ULONG GetOffset(PVOID virtualAddress);
ULONG GetPageTableIndex(PVOID virtualaddr);
ULONG GetPageDirectoryIndex(PVOID virtualaddr);
ULONG GetPageDirectoryPointerIndex(PVOID virtualaddr);
PPDE GetPdeAddress(PVOID VirtualAddress);
PPTE GetPteAddress(PVOID VirtualAddress);
ULONG GetPhysAddress(PVOID virtualaddr);
NTSTATUS Remap(PVOID kmdfDataPointer, PVOID clientDataPointer);

VOID printPteHeader();
VOID printPdeHeader();
VOID printPpte(PPTE ppte);
VOID printPpde(PPDE ppde);
#endif
