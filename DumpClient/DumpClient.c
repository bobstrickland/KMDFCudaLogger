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



int main(int argc, _TCHAR* argv[]) {
#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
	PSHARED_MEMORY_STRUCT SharedMemory;

	SharedMemory = (PSHARED_MEMORY_STRUCT)malloc(sizeof(PSHARED_MEMORY_STRUCT));

	if (argc > 3 && strncmp(argv[1], "0x", 2) == 0) {
		printf("Control Buffer Location Offset: 0x%s Memory Window Offset: 0x%s\n", argv[2], argv[3]);
		SharedMemory->BufferOffset = HexStringToUInt(argv[2]);
		SharedMemory->WindowOffset = HexStringToUInt(argv[3]);
	}
	else if (argc > 2) {
		printf("C_ontrol Buffer Location Offset: %s Memory Window Offset: %s\n", argv[1], argv[2]);
		SharedMemory->BufferOffset = atol(argv[1]);
		SharedMemory->WindowOffset = atol(argv[2]);
	}
	else if (argc > 1) {
		printf("Control Buffer Location Offset: 0x1700 Memory Window Offset: %s\n", argv[1]);
		SharedMemory->BufferOffset = 0x1700;
		SharedMemory->WindowOffset = atol(argv[1]);
	}
	else {

		PCHAR offsetText;

		offsetText = (PCHAR)malloc(sizeof(CHAR) * 64);
		printf("Enter Window Offset: ");
		gets_s(offsetText, 64);
		SharedMemory->WindowOffset = atol(offsetText);
		//printf("Enter Buffer Offset: ");
		//gets_s(offsetText, 64);
		//SharedMemory->BufferOffset = atol(offsetText);
		SharedMemory->BufferOffset = 0x1700;
	}

	hControlDevice = CreateFile(TEXT("\\\\.\\DeviceMemDump"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hControlDevice) {
		printf("Failed to open DeviceMemDump device\n");
	}
	else {
		ULONG      BytesReturned;
		PCHAR      ClientMemory;
		OVERLAPPED DeviceIoOverlapped;
		
		ClientMemory = (PCHAR)malloc(ClientMemoryLength);

		SharedMemory->ClientMemory = ClientMemory;

		DeviceIoOverlapped.Offset = 0;
		DeviceIoOverlapped.OffsetHigh = 0;
		DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		printf("\nTransmitting\n");
		if (!DeviceIoControl(hControlDevice, IOCTL_CUSTOM_CODE, NULL, 0, SharedMemory, SharedMemoryLength, &BytesReturned, &DeviceIoOverlapped)) {
			printf("Ioctl to DeviceMemDump device failed.\n");
		}
		else {
			printf("Ioctl to DeviceMemDump device succeeded.\nbuffer is: [");
			if (TRUE) {
				for (int i = 0; i < PRAMIN_LENGTH; i++) {
					printf("%c", ClientMemory[i]);
				}
				printf("]\nalso: [");
				for (int i = 0; i < PRAMIN_LENGTH; i++) {
					printf("0x%x ", ClientMemory[i]);
					if (i % 20 == 0) {
						printf("]\n[");
					}
				}
			}
			printf("]\n");
		}
		CloseHandle(hControlDevice);
	}
	printf("fin\n");
	return 0;
}

