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



#define FILE_DEVICE_WINIO 0x00008010
#define PRAMIN_LENGTH     0x100000
#define WINIO_IOCTL_INDEX 0x810
#define IOCTL_WINIO_READPORT		 CTL_CODE(FILE_DEVICE_WINIO,  \
	WINIO_IOCTL_INDEX + 4,   \
	METHOD_BUFFERED,         \
	FILE_ANY_ACCESS)

ULONG Bar0Address;
ULONG HostMemAddress;
ULONG WindowAddress;


PULONG HostMem;
PULONG Pramin;

//PULONG HostMemPointer;
//PULONG WindowPointer;

typedef struct _tagPortStruct
{
	USHORT wPortAddr;
	ULONG dwPortVal;
	UCHAR bSize;
} tagPortStruct, *ptagPortStruct;

typedef struct _SHARED_MEMORY_STRUCT
{
	ULONG BufferOffset;
	PCHAR ClientMemory;
} SHARED_MEMORY_STRUCT, *PSHARED_MEMORY_STRUCT;


//ULONG SharedMemoryLength = sizeof(SHARED_MEMORY_STRUCT);

typedef struct _CONTROL_DEVICE_EXTENSION {
	PVOID   ControlData;
} CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, ControlGetData)

EVT_WDF_IO_QUEUE_IO_DEFAULT ReadDeviceMemory;

NTSTATUS CreateControlDevice(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath);

VOID ReadDeviceMemory(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request);
NTSTATUS ReadWriteConfigSpace(IN PDEVICE_OBJECT DeviceObject, IN ULONG ReadOrWrite, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length);
NTSTATUS ReadConfigSpace(IN PDEVICE_OBJECT DeviceObject, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length);
NTSTATUS WriteConfigSpace(IN PDEVICE_OBJECT DeviceObject, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length);
VOID pauseForABit(CSHORT secondsDelay);

#endif
