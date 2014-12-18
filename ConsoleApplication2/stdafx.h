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


#define KEY_MAKE  0
#define KEY_BREAK 1
#define INVALID 0X00 //scan code not supported by this driver   
#define SPACE 0X01 //space bar   
#define ENTER 0X02 //enter key   
#define LSHIFT 0x03 //left shift key   
#define RSHIFT 0x04 //right shift key   
#define CTRL  0x05 //control key   
#define ALT   0x06 //alt key   
char KeyMap[84] = {
	INVALID, //0   
	INVALID, //1   
	'1', //2   
	'2', //3   
	'3', //4   
	'4', //5   
	'5', //6   
	'6', //7   
	'7', //8   
	'8', //9   
	'9', //A   
	'0', //B   
	'-', //C   
	'=', //D   
	INVALID, //E   
	INVALID, //F   
	'q', //10   
	'w', //11   
	'e', //12   
	'r', //13   
	't', //14   
	'y', //15   
	'u', //16   
	'i', //17   
	'o', //18   
	'p', //19   
	'[', //1A   
	']', //1B   
	ENTER, //1C   
	CTRL, //1D   
	'a', //1E   
	's', //1F   
	'd', //20   
	'f', //21   
	'g', //22   
	'h', //23   
	'j', //24   
	'k', //25   
	'l', //26   
	';', //27   
	'\'', //28   
	'`', //29   
	LSHIFT, //2A   
	'\\', //2B   
	'z', //2C   
	'x', //2D   
	'c', //2E   
	'v', //2F   
	'b', //30   
	'n', //31   
	'm', //32   
	',', //33   
	'.', //34   
	'/', //35   
	RSHIFT, //36   
	INVALID, //37   
	ALT, //38   
	SPACE, //39   
	INVALID, //3A   
	INVALID, //3B   
	INVALID, //3C   
	INVALID, //3D   
	INVALID, //3E   
	INVALID, //3F   
	INVALID, //40   
	INVALID, //41   
	INVALID, //42   
	INVALID, //43   
	INVALID, //44   
	INVALID, //45   
	INVALID, //46   
	'7', //47   
	'8', //48   
	'9', //49   
	INVALID, //4A   
	'4', //4B   
	'5', //4C   
	'6', //4D   
	INVALID, //4E   
	'1', //4F   
	'2', //50   
	'3', //51   
	'0', //52   
};



