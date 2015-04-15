// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <stdio.h>
#include <tchar.h>
#include <ntstatus.h>

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef void *  PVOID;


#define PRAMIN_LENGTH 0x100000
#define PRAMIN_PAGES 0x4000
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _SHARED_MEMORY_STRUCT
{
	ULONG BufferOffset;
	PCHAR ClientMemory;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;

NTSTATUS dumpMemoryByOffset(HANDLE hControlDevice, ULONG BufferOffset);
NTSTATUS readMemoryByOffset(HANDLE hControlDevice, PSHARED_MEMORY_STRUCT SharedMemory);
NTSTATUS dumpMemoryToFile(HANDLE hControlDevice, const char * FileName);
