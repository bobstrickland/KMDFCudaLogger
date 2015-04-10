// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

//#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>
#include <ntstatus.h>

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef void *  PVOID;


#define VIDEO_MEMORY_SIZE 1024000000
#define PRAMIN_LENGTH 0x100000
#define PRAMIN_PAGES (VIDEO_MEMORY_SIZE/PRAMIN_LENGTH)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _SHARED_MEMORY_STRUCT
{
	ULONG BufferOffset;
	PCHAR ClientMemory;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;


ULONG ClientMemoryLength = (0x100000 * sizeof(CHAR));
ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);


NTSTATUS dumpMemoryByOffset(HANDLE hControlDevice, ULONG BufferOffset);
NTSTATUS readMemoryByOffset(HANDLE hControlDevice, PSHARED_MEMORY_STRUCT SharedMemory);
NTSTATUS dumpMemoryToFile(HANDLE hControlDevice, const char * FileName);
