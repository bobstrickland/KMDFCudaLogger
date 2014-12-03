#ifndef __SharedHeader_h__ 
#define __SharedHeader_h__ 


typedef struct _SHARED_MEMORY_STRUCT
{
	HANDLE ClientProcessHandle;
	PVOID  ClientMemory;
	PPTE PageTable;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;


ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);

#endif
