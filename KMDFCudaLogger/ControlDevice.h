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






#endif
