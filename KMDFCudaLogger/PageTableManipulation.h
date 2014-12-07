#ifndef __PageTableManipulation_h__ 
#define __PageTableManipulation_h__ 

#include <ntdef.h>
#include <ntddk.h>
#include <wdf.h>

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
			// Protection
			ULONG Accessed : 1;
			ULONG Dirty : 1;
			ULONG LargePage : 1;
			ULONG Global : 1;
			ULONG CopyOnWrite : 1;
			ULONG Prototype : 1;
			ULONG Transition : 1;
			ULONG PageFrameNumber : 20;
		} DUMMYSTRUCTNAME;
	};
} PTE, *PPTE;








typedef struct _NEX_FAST_REF
{
	union
	{
		PVOID Object;
		ULONG RefCnt : 3;
		ULONG Value;
	};
} NEX_FAST_REF, *PNEX_FAST_REF;
/**/
typedef struct _NEX_PUSH_LOCK
{
	union
	{
		ULONG Locked : 1;
		ULONG Waiting : 1;
		ULONG Waking : 1;
		ULONG MultipleShared : 1;
		ULONG Shared : 28;
		ULONG Value;
		PVOID Ptr;
	};
} NEX_PUSH_LOCK, *PNEX_PUSH_LOCK;
/**/

typedef struct _NKGDTENTRY
{
	USHORT LimitLow;
	USHORT BaseLow;
	ULONG HighWord;
} NKGDTENTRY, *PNKGDTENTRY;

typedef struct _NKIDTENTRY
{
	USHORT Offset;
	USHORT Selector;
	USHORT Access;
	USHORT ExtendedOffset;
} NKIDTENTRY, *PNKIDTENTRY;

typedef struct _NKEXECUTE_OPTIONS
{
	ULONG ExecuteDisable : 1;
	ULONG ExecuteEnable : 1;
	ULONG DisableThunkEmulation : 1;
	ULONG Permanent : 1;
	ULONG ExecuteDispatchEnable : 1;
	ULONG ImageDispatchEnable : 1;
	ULONG Spare : 2;
} NKEXECUTE_OPTIONS, *PNKEXECUTE_OPTIONS;


typedef struct _NKPROCESS
{
	DISPATCHER_HEADER Header;
	LIST_ENTRY ProfileListHead;
	ULONG DirectoryTableBase;
	ULONG Unused0;
	NKGDTENTRY LdtDescriptor;
	NKIDTENTRY Int21Descriptor;
	USHORT IopmOffset;
	UCHAR Iopl;
	UCHAR Unused;
	ULONG ActiveProcessors;
	ULONG KernelTime;
	ULONG UserTime;
	LIST_ENTRY ReadyListHead;
	SINGLE_LIST_ENTRY SwapListEntry;
	PVOID VdmTrapcHandler;
	LIST_ENTRY ThreadListHead;
	ULONG ProcessLock;
	ULONG Affinity;
	union
	{
		ULONG AutoAlignment : 1;
		ULONG DisableBoost : 1;
		ULONG DisableQuantum : 1;
		ULONG ReservedFlags : 29;
		LONG ProcessFlags;
	};
	CHAR BasePriority;
	CHAR QuantumReset;
	UCHAR State;
	UCHAR ThreadSeed;
	UCHAR PowerState;
	UCHAR IdealNode;
	UCHAR Visited;
	union
	{
		NKEXECUTE_OPTIONS Flags;
		UCHAR ExecuteOptions;
	};
	ULONG StackCount;
	LIST_ENTRY ProcessListEntry;
	UINT64 CycleTime;
} NKPROCESS, *PNKPROCESS;


typedef struct _NEPROCESS
{
	NKPROCESS Pcb;
	NEX_PUSH_LOCK ProcessLock;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	EX_RUNDOWN_REF RundownProtect;
	PVOID UniqueProcessId;
	LIST_ENTRY ActiveProcessLinks;
	ULONG QuotaUsage[3];
	ULONG QuotaPeak[3];
	ULONG CommitCharge;
	ULONG PeakVirtualSize;
	ULONG VirtualSize;
	LIST_ENTRY SessionProcessLinks;
	PVOID DebugPort;
	union
	{
		PVOID ExceptionPortData;
		ULONG ExceptionPortValue;
		ULONG ExceptionPortState : 3;
	};
	PVOID ObjectTable;
	NEX_FAST_REF Token;
	ULONG WorkingSetPage;
	NEX_PUSH_LOCK AddressCreationLock;
	PETHREAD RotateInProgress;
	PETHREAD ForkInProgress;
	ULONG HardwareTrigger;
	PVOID PhysicalVadRoot;
	PVOID CloneRoot;
	ULONG NumberOfPrivatePages;
	ULONG NumberOfLockedPages;
	PVOID Win32Process;
	PVOID Job;
	PVOID SectionObject;
	PVOID SectionBaseAddress;
	PVOID QuotaBlock;
	PVOID WorkingSetWatch;
	PVOID Win32WindowStation;
	PVOID InheritedFromUniqueProcessId;
	PVOID LdtInformation;
	PVOID VadFreeHint;
	PVOID VdmObjects;
	PVOID DeviceMap;
	PVOID EtwDataSource;
	PVOID FreeTebHint;
	union
	{
		PTE PageDirectoryPte;
		UINT64 Filler;
	};
} NEPROCESS, *PNEPROCESS;









#define PTE_SIZE 8
#define PAGE_SIZE 0x1000

#define	PROCESS_PAGE_DIRECTORY_BASE		0xC0600000 
#define PROCESS_PAGE_TABLE_BASE			0xC0000000 

PPTE GetPteAddress(PVOID VirtualAddress);
ULONG GetPageDirectoryBaseRegister();
PHYSICAL_ADDRESS GetPDBRPhysicalAddress();
ULONG GetPhysAddress(void * virtualaddr);
ULONG GetPhysAddressPhysically(void * virtualaddr);
ULONG GetPhysAddressPhysicallyWithProcessHandle(PVOID virtualaddr, HANDLE processHandle);
ULONG GetPhysAddressPhysicallyWithProcess(PVOID virtualaddr, PNEPROCESS peProcess);
ULONG GetPhysAddressPhysicallyWithPDPT(PVOID virtualaddr, PHYSICAL_ADDRESS pageDirPointerTablePA);
#endif
