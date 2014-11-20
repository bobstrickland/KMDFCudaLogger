#ifndef __ControlDevice_h__ 
#define __ControlDevice_h__ 
#include <hidpddi.h>
#include <wmilib.h>
#include <ntddk.h>
#include <ntddkbd.h>   

#include <wdf.h>
#include <wdmsec.h> // for SDDLs
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <Driver.h>



typedef struct _CONTROL_DEVICE_EXTENSION {
	PVOID   ControlData;
} CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, ControlGetData)

EVT_WDF_IO_QUEUE_IO_DEFAULT ReadKeyboardBuffer;





NTSTATUS CreateControlDevice(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath);

VOID ReadKeyboardBuffer(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request);




typedef struct _PTE
{
	ULONG Present : 1;
	ULONG Writable : 1;
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	ULONG Accessed : 1;
	ULONG Dirty : 1;
	ULONG LargePage : 1;
	ULONG Global : 1;
	ULONG ForUse1 : 1;
	ULONG ForUse2 : 1;
	ULONG ForUse3 : 1;
	ULONG PageFrameNumber : 20;
} PTE, *PPTE;

#define PDE_OFFSET 0xC0300000
#define PTE_OFFSET 0xC0000000

//PDEaddress = ((VirtualAddress >> 20) & (~0x3)) + 0xC0300000
//PTEaddress = ((VirtualAddress >> 10) & (~0x3)) + 0xC0000000
//#add correction(~0x3) also here!
#define PDEaddr(VirtualAddress) ( (PPTE) (( ((ULONG) VirtualAddress) >> 20) + PDE_OFFSET) )
#define PTEaddr(VirtualAddress) ( (PPTE) (( ((ULONG) VirtualAddress) >> 10) + PTE_OFFSET) )



#endif