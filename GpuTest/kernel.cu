#include <windows.h>
#include <winbase.h>
//#include "stdafx.h"
#include <stdio.h>
#include <kernelspecs.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "helper_cuda.h"
#include "device_launch_parameters.h"
#include <DbgEng.h>
#include <process.h>
#include <psapi.h>
#include <time.h>

#include <winsock.h>
/**/



__global__ void copyKeyboardBuffer(PCHAR inBuffer, PCHAR outBuffer) {
	int index = blockIdx.x*blockDim.x + threadIdx.x;
	outBuffer[index] = inBuffer[index];
}

__global__ void sendTextToGpu(PCHAR keyboardText) {
	printf("In GPU\n");
	printf("GPU: [0x%lx]\n", keyboardText);
	printf("GPU: %s\n", keyboardText);
}

int main(int argc, char * argv[]) {


	PCHAR textForGpu = (PCHAR)malloc(sizeof(CHAR) * 2048);


	PCHAR d_OutgoingKeystrokeBuffer;
	PCHAR h_OutgoingKeystrokeBuffer;


	h_OutgoingKeystrokeBuffer = (PCHAR)malloc(sizeof(CHAR) * 2048);



	checkCudaErrors(cudaSetDevice(0));
	checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));
	checkCudaErrors(cudaHostRegister(h_OutgoingKeystrokeBuffer, sizeof(CHAR) * 2048, cudaHostRegisterMapped)); // zzy
	checkCudaErrors(cudaHostGetDevicePointer((void **)&d_OutgoingKeystrokeBuffer, (void *)h_OutgoingKeystrokeBuffer, 0)); // zzy

	while (TRUE) {
		printf("enter text:");
		gets_s(textForGpu, 2048);
		size_t textLength = strlen(textForGpu);

		printf("got %d characters\n", textLength);
		if (textLength == 0) {
			break;
		}
		else {
			textLength++;
			PCHAR d_Text;
			checkCudaErrors(cudaMalloc((void **)&d_Text, sizeof(char) * textLength));
			checkCudaErrors(cudaMemcpy(d_Text, textForGpu, textLength * sizeof(char), cudaMemcpyHostToDevice));
			printf("Launching CUDA process.\n");

			
			copyKeyboardBuffer<<<4, 512 >>>(d_Text, d_OutgoingKeystrokeBuffer);
			checkCudaErrors(cudaDeviceSynchronize());

			//printf("is this the right string? %s\n", h_OutgoingKeystrokeBuffer);
			sendTextToGpu <<<1, 1 >>>(d_Text);
			checkCudaErrors(cudaDeviceSynchronize());
		}
	}
	return 0;
}




