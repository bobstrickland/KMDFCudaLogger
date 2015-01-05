#ifndef __SharedHeader_h__ 
#define __SharedHeader_h__ 


#ifndef CHAR
typedef char CHAR;
#endif

typedef struct _SHARED_MEMORY_STRUCT
{
	PVOID ClientMemory;
	CHAR  instruction;
	BOOLEAN largePage;
	ULONG offset;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;


ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);

#endif
