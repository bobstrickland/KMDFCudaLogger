#ifndef __Driver_h__ 
#define __Driver_h__ 

typedef struct _USBK_DATA {
	PVOID pointer0;
	PVOID pointer1;
	ULONG long0;
	ULONG long1;
	ULONG long2;
	PVOID pointer2;
	PVOID pointer3;
	ULONG long3;
	PVOID pointer4;
	PVOID pointer5;
	ULONG long4;
	ULONG long5;
	ULONG long6;
	ULONG long7;
	ULONG long8;
	PVOID pointer6;
	PVOID pointer7;
	ULONG long9;
	ULONG longA;
	ULONG longB;
	ULONG longC;
	ULONG longD;
	PVOID pointer8;
	PVOID pointer9;
	ULONG longE;
	ULONG longF;
	ULONG long10;
	PVOID pointerA;
	PVOID pointerB;
	ULONG buffer;
} USBK_DATA, *PUSBK_DATA;

typedef struct _USBK_EXT {
	PVOID pointer0;
	PVOID pointer1;
	PVOID pointer2;
	ULONG long0;
	ULONG long1;
	ULONG long2;
	ULONG long3;
	PVOID pointer3;
	PVOID pointer4;
	ULONG long4;
	ULONG long5;
	PVOID pointer5;
	PUSBK_DATA dataPointer;
} USBK_EXT, *PUSBK_EXT;

//////////////////////////////////////////////////////////
// Undocumented Kernel calls to get the KbdClass driver //
//////////////////////////////////////////////////////////
NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(IN PUNICODE_STRING ObjectName, IN ULONG Attributes, IN PACCESS_STATE PassedAccessState OPTIONAL, IN ACCESS_MASK DesiredAccess OPTIONAL, IN POBJECT_TYPE ObjectType, IN KPROCESSOR_MODE AccessMode, IN OUT PVOID ParseContext OPTIONAL, OUT PVOID * Object);
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE ObGetObjectType(PVOID Object);
//////////////////////////////////////////////////////////

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS GetKeyboardMemoryBuffer(IN PDRIVER_OBJECT pDriverObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID Unload(IN PDRIVER_OBJECT pDriverObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID NdisMSleep(IN    ULONG    MicrosecondsToSleep);

#endif
