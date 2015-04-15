// DumpClient.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include "stdafx.h"
//#include <ntsecapi.h>

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



int main(int argc, const char * argv[]) {
#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
	NTSTATUS status = STATUS_UNHANDLED_EXCEPTION;

	hControlDevice = CreateFile(TEXT("\\\\.\\DeviceMemDump"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hControlDevice) {
		printf("Failed to open DeviceMemDump device\n");
		status = STATUS_INVALID_HANDLE;
	}
	else {
		if (argc > 2 && strncmp(argv[1], "0x", 2) == 0) {
			printf("Hex: [%s]\n", argv[2]);
			ULONG BufferOffset = HexStringToUInt(argv[2]);
			status = dumpMemoryByOffset(hControlDevice, BufferOffset);
		}
		else if (argc > 2 && strncmp(argv[1], "file", 4) == 0) {
			printf("Everything to file\n");
			status = dumpMemoryToFile(hControlDevice, argv[2]);
		}
		else if (argc > 1) {
			ULONG BufferOffset = atol(argv[1])*PRAMIN_LENGTH;
			printf("Buffer page [%s] Memory offset [0x%lx]\n", argv[1], BufferOffset);
			status = dumpMemoryByOffset(hControlDevice, BufferOffset);
		}
		CloseHandle(hControlDevice);
	}
	printf("fin\n");
	return 0;
}

NTSTATUS dumpMemoryToFile(HANDLE hControlDevice, const char * FileName) {

	FILE * FilePointer;
	printf("Opening Output file...");
	errno_t errorCode = fopen_s(&FilePointer, FileName, "w");
	printf("Open.\n");

	PSHARED_MEMORY_STRUCT SharedMemory;

	ULONG      BytesReturned;
	PCHAR      ClientMemory;
	PCHAR      MemoryMemory;
	OVERLAPPED DeviceIoOverlapped;
	NTSTATUS status = STATUS_UNHANDLED_EXCEPTION;

	SharedMemory = (PSHARED_MEMORY_STRUCT)malloc(sizeof(PSHARED_MEMORY_STRUCT));
	ClientMemory = (PCHAR)malloc(PRAMIN_LENGTH);
	MemoryMemory = (PCHAR)malloc(PRAMIN_LENGTH);

	SharedMemory->ClientMemory = ClientMemory;

	DeviceIoOverlapped.Offset = 0;
	DeviceIoOverlapped.OffsetHigh = 0;
	DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	printf("Writing pages 0-%d", PRAMIN_PAGES);
	for (ULONG BufferOffset = 0; BufferOffset < PRAMIN_PAGES;) {
		SharedMemory->BufferOffset = BufferOffset;

		status = readMemoryByOffset(hControlDevice, SharedMemory);
		if (BufferOffset == 0) {
			memcpy(MemoryMemory, ClientMemory, PRAMIN_LENGTH);
		}
		else {
			for (int i = 0; i < PRAMIN_LENGTH; i++) {
				if (MemoryMemory[i] != ClientMemory[i]) {
					MemoryMemory[i] = 0;
				}
			}
		}
		if (NT_SUCCESS(status)) {
			fwrite(ClientMemory, PRAMIN_LENGTH, 1, FilePointer);
			printf(".");
		}
		else {
			printf("Call failed. [0x%lx]\n", status);
			break;
		}
		BufferOffset = BufferOffset + 0x10;
	}
	printf("finished writing.\nClosing...");
	fclose(FilePointer);
	printf("Closed.");

	ULONG NonZeroByteCount = 0;
	for (int i = 0; i < PRAMIN_LENGTH; i++) {
		if (MemoryMemory[i] != 0) {
			if (NonZeroByteCount == 0) {
				printf("\n\nFound unchanged memory:\n");
			}
			printf("%c", MemoryMemory[i]);
			NonZeroByteCount++;
		}
	}
	if (NonZeroByteCount > 0) {
		printf("\nThere are %lu bytes found that never change.  Percentage is %lu\n", NonZeroByteCount, ((NonZeroByteCount * 100) / PRAMIN_LENGTH));
	}
	printf("\nFinished.\n");
	return status;
}

NTSTATUS dumpMemoryByOffset(HANDLE hControlDevice, ULONG BufferOffset) {
	PSHARED_MEMORY_STRUCT SharedMemory;

	ULONG      BytesReturned;
	PCHAR      ClientMemory;
	OVERLAPPED DeviceIoOverlapped;
	NTSTATUS status = STATUS_UNHANDLED_EXCEPTION;

	SharedMemory = (PSHARED_MEMORY_STRUCT)malloc(sizeof(PSHARED_MEMORY_STRUCT));
	ClientMemory = (PCHAR)malloc(PRAMIN_LENGTH);

	SharedMemory->ClientMemory = ClientMemory;
	SharedMemory->BufferOffset = BufferOffset;

	DeviceIoOverlapped.Offset = 0;
	DeviceIoOverlapped.OffsetHigh = 0;
	DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	status = readMemoryByOffset(hControlDevice, SharedMemory);
	if (NT_SUCCESS(status)) {
		for (ULONG i = 0; i < PRAMIN_LENGTH; i++) {
			printf("%c", ClientMemory[i]);
			if ((i + 1) % 128 == 0) {
				printf("\n");
			}
		}
		printf("\nIn HEX:\n[0x0] [");
		for (ULONG i = 0; i < PRAMIN_LENGTH; i++) {
			printf("0x%x ", ClientMemory[i]);
			if ((i + 1) % 16 == 0) {
				printf("]\n[0x%lx] [", i);
			}
		}
		printf("]\n");
	}
	else {
		printf("Call failed. [0x%lx]\n", status);
	}
	return status;
}

NTSTATUS readMemoryByOffset(HANDLE hControlDevice, PSHARED_MEMORY_STRUCT SharedMemory) {

	ULONG      BytesReturned;
	OVERLAPPED DeviceIoOverlapped;
	NTSTATUS status = STATUS_UNHANDLED_EXCEPTION;

	memset(SharedMemory->ClientMemory, 0, PRAMIN_LENGTH);

	DeviceIoOverlapped.Offset = 0;
	DeviceIoOverlapped.OffsetHigh = 0;
	DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, SharedMemory, sizeof(SHARED_MEMORY_STRUCT), &BytesReturned, &DeviceIoOverlapped)) {
		status = STATUS_IO_DEVICE_ERROR;
	}
	else {
		status = STATUS_SUCCESS;
	}
	return status;
}


