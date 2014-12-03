// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
//#include <ntddkbd.h>
#include <SharedHeader.h>
//#include <ntdef.h>



PVOID GetPageDirectoryBaseRegister()
{
	PVOID returnValue = NULL;
	__asm
	{
			pushad                // save the registers
			mov eax, cr3          // read the Page Directory Base Register from CR3
			mov returnValue, eax  // set returnValue with that value
			popad                 // restore the registers
	}
	return returnValue;
}



int main(int argc, _TCHAR* argv[]) {

	//PVOID foo = PsGetCurrentProcess();

	if (FALSE) {

		PKEYBOARD_INPUT_DATA keyboardData;

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));

		PPTE ppte = GetPteAddress(keyboardData);
		printf("PTPE is [0x%lx]\n", ppte);

		ULONG pageDirectoryIndex = (ULONG)keyboardData >> 21;
		ULONG pageTableIndex = (ULONG)keyboardData >> 12 & 0x01FF;
		ULONG offset = (ULONG)keyboardData & 0x0fff;

		ULONG ptPFN = ppte->PageFrameNumber;
		ULONG baseAddress = ptPFN << 12;
		ULONG finalPhysicalAddress = baseAddress + offset;
		printf("  PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", ptPFN, baseAddress, finalPhysicalAddress);
		printf("Physical address for [0x%lx] is [0x%lx]\n", keyboardData, finalPhysicalAddress);
	}




//#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	HANDLE hControlDevice;
	ULONG  bytes;

	//
	// Open handle to the control device. Please note that even
	// a non-admin user can open handle to the device with
	// FILE_READ_ATTRIBUTES | SYNCHRONIZE DesiredAccess and send IOCTLs if the
	// IOCTL is defined with FILE_ANY_ACCESS. So for better security avoid
	// specifying FILE_ANY_ACCESS in your IOCTL defintions.
	// If the IOCTL is defined to have FILE_READ_DATA access rights, you can
	// open the device with GENERIC_READ and call DeviceIoControl.
	// If the IOCTL is defined to have FILE_WRITE_DATA access rights, you can
	// open the device with GENERIC_WRITE and call DeviceIoControl.
	//
	hControlDevice = CreateFile(TEXT("\\\\.\\EvilFilter"),
		GENERIC_READ | GENERIC_WRITE, // Only read access
		FILE_SHARE_READ | FILE_SHARE_WRITE, //0, // FILE_SHARE_READ | FILE_SHARE_WRITE
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		FILE_FLAG_OVERLAPPED, // 0 //  No special attributes
		NULL); // No template file

	if (INVALID_HANDLE_VALUE == hControlDevice) {
		printf("Failed to open EvilFilter device\n");
	}
	else {

		PKEYBOARD_INPUT_DATA keyboardData;
		OVERLAPPED      DeviceIoOverlapped;
		PSHARED_MEMORY_STRUCT dataToTransmit;
		HANDLE currentProcessHandle;

		currentProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(sizeof(KEYBOARD_INPUT_DATA));
		dataToTransmit = (PSHARED_MEMORY_STRUCT)malloc(SharedMemoryLength);

		PPTE ppte = GetPteAddress(keyboardData);
		printf("PTPE is ", ppte);

		dataToTransmit->ClientProcessHandle = currentProcessHandle;
		dataToTransmit->ClientMemory = keyboardData;
		dataToTransmit->PageTable = ppte;

		DeviceIoOverlapped.Offset = 0;
		DeviceIoOverlapped.OffsetHigh = 0;
		DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


		keyboardData->MakeCode = 'Z';
		if (!DeviceIoControl(hControlDevice,
			IOCTL_CUSTOM_CODE,
			NULL, 0,
			dataToTransmit, SharedMemoryLength,
			&bytes, &DeviceIoOverlapped)) {
			printf("Ioctl to EvilFilter device failed\n");
		}
		else {
			printf("Ioctl to EvilFilter device succeeded\n");
			printf("keyboardData is [0x%lx]\n", keyboardData);


		}
		CloseHandle(hControlDevice);
	}


	/**/
	printf("begin loop\n");

	SYSTEMTIME systemTime;

	GetSystemTime(&systemTime);
	WORD CurrentMinute = systemTime.wMinute;
	WORD ExitMinute = systemTime.wMinute + 7;
	while (TRUE) {
//		if (keyboardData->Flags == 1) {
//			printf("keyboardData->Make Code is [%x]\n", keyboardData->MakeCode);
//		}
		GetSystemTime(&systemTime);
		CurrentMinute = systemTime.wMinute;
	}


	printf("end loop\n");
	/**/
	return 0;
}


/** /
PPTE GetPteAddress(PVOID VirtualAddress) {

KdPrint(("IN GetPteAddress :: KIRQL is now %x\n", KeGetCurrentIrql());
PPTE pPTE = 0;
__asm
{
cli                     //disable interrupts
pushad
mov esi, PROCESS_PAGE_DIR_BASE
mov edx, VirtualAddress
mov eax, edx
shr eax, 22 // GOOD TO HERE
lea eax, [esi + eax * 4]  //pointer to page directory entry
//				test[eax], 0x80        //is it a large page?
//				jnz Is_Large_Page       //it's a large page
mov esi, PROCESS_PAGE_TABLE_BASE
shr edx, 12
lea eax, [esi + edx * 4]  //pointer to page table entry (PTE)
mov pPTE, eax
jmp Done

//NOTE: There is not a page table for large pages because
//the phys frames are contained in the page directory.
Is_Large_Page :
mov pPTE, eax

Done :
popad
sti                    //reenable interrupts
}//end asm
return pPTE;

}//end GetPteAddress
/**/

/**************************************************************************
* GetPhysicalFrameAddress - Gets the base physical address in memory where
*                           the page is mapped. This corresponds to the
*                           bits 12 - 32 in the page table entry.
*
* Parameters -
*       PPTE pPte - Pointer to the PTE that you wish to retrieve the
*       physical address from.
*
* Return - The physical address of the page.
**************************************************************************/
/** /
ULONG GetPhysicalFrameAddress(PPTE pPte)
{
ULONG Frame = 0;

__asm
{
cli
pushad
mov eax, pPte
mov ecx, [eax]
shr ecx, 12  //physical page frame consists of the
//upper 20 bits
mov Frame, ecx
popad
sti
}//end asm
return Frame;

}//end GetPhysicalFrameAddress
/**/


PPTE GetPteAddress(PVOID virtualaddr) {

	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	printf("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	printf("Looking conventional \n");

	PPTE pageDirectoryTable = (PPTE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	printf("pageDirectoryTable   [0x%lx] ", pageDirectoryTable);
	if ((pageDirectoryTable)) {
		//printf("[0x%lx] ", MmGetPhysicalAddress(pageDirectoryTable));
//		ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
//		printf("  PageFrameNumber is [0x%lx]\n", pdPFN);
		PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
		printf("pageTable   [0x%lx] \n", pageTable);
		if ((pageTable)) {
			return pageTable;
		}
		else {
			printf(" is INVALID\n");
			return NULL;
		}
	}
	else {
		printf(" is INVALID\n");
		return NULL;
	}
}

