#ifndef __Driver_h__ 
#define __Driver_h__ 

#define INITGUID

#include <ntddk.h>
#include <wdf.h>


//
// WDFDRIVER Events
//

PDRIVER_OBJECT pciDriverObject;
PDEVICE_OBJECT pciVideoDeviceObject;

NTSTATUS GetPciDeviceByName(_In_ PDRIVER_OBJECT  pDriverObject, PCWSTR driverName);
NTSTATUS GetPciDevice(_In_ PDRIVER_OBJECT  pDriverObject);
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KMDFMemoryDumpEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP KMDFMemoryDumpEvtDriverContextCleanup;


NTKERNELAPI NTSTATUS ObReferenceObjectByName(IN PUNICODE_STRING ObjectName, IN ULONG Attributes, IN PACCESS_STATE PassedAccessState OPTIONAL, IN ACCESS_MASK DesiredAccess OPTIONAL, IN POBJECT_TYPE ObjectType, IN KPROCESSOR_MODE AccessMode, IN OUT PVOID ParseContext OPTIONAL, OUT PVOID * Object);
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE ObGetObjectType(PVOID Object);

#endif
