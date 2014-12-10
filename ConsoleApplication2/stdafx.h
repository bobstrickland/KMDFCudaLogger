// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>

typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef void *  PVOID;

/** /
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
} PTE, *PPTE;
/**/
typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

typedef struct _LLIST {
	struct _LLIST *previous;
	PKEYBOARD_INPUT_DATA keyboardBuffer;
} LLIST, *PLLIST;

PVOID GetPdeAddress(PVOID virtualaddr);
PVOID GetPteAddress(PVOID virtualaddr);

//#define PTE_SIZE 8
#define PTE_SIZE 4
#define PAGE_SIZE 0x1000

//#define	PROCESS_PAGE_DIRECTORY_BASE		0xC0600000 
#define	PROCESS_PAGE_DIRECTORY_BASE		0xC0300000 
#define PROCESS_PAGE_TABLE_BASE			0xC0000000 
// TODO: reference additional headers your program requires here
