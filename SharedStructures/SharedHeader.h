#ifndef __SharedHeader_h__ 
#define __SharedHeader_h__ 




typedef struct _SHARED_MEMORY_STRUCT
{
#ifndef _WIN64
	PVOID  ClientMemory;
	PVOID PageDirectory;
	PVOID PageTable;
	CHAR instruction;
	ULONG offset;
#else
	PVOID  ClientMemory;
	PVOID PageDirectory;
	PVOID PageTable;
	CHAR instruction;
	ULONGLONG offset;
#endif

} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;


ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);

#endif
