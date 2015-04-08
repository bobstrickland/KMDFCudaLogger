// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef void *  PVOID;



typedef struct _SHARED_MEMORY_STRUCT
{
	ULONG BufferOffset;
	ULONG WindowOffset;
	PCHAR ClientMemory;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;

ULONG PRAMIN_LENGTH = 0x100000;
ULONG ClientMemoryLength = (0x100001 * sizeof(CHAR));
ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);
// TODO: reference additional headers your program requires here
