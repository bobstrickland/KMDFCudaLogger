// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <ntddkbd.h>
//#include <ntdef.h>


int main(int argc, _TCHAR* argv[]) {

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

		PKEYBOARD_INPUT_DATA keyboardData = NULL;
		OVERLAPPED      DeviceIoOverlapped;

		ULONG length = sizeof(PKEYBOARD_INPUT_DATA);

		keyboardData = (PKEYBOARD_INPUT_DATA)malloc(length);
		DeviceIoOverlapped.Offset = 0;
		DeviceIoOverlapped.OffsetHigh = 0;
		DeviceIoOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


		printf("keyboardData  size is now [%lu]\n", length);
		keyboardData->MakeCode = 'Z';
		if (!DeviceIoControl(hControlDevice,
			IOCTL_CUSTOM_CODE,
			NULL, 0,
			keyboardData, length,
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



